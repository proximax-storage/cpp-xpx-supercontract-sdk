/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "DefaultBatchesManager.h"
#include <virtualMachine/ExecutionErrorConidition.h>
#include <crypto/Hashes.h>
#include "AutorunBlockchainQueryHandler.h"

namespace sirius::contract {

DefaultBatchesManager::DefaultBatchesManager(uint64_t nextBatchIndex, ContractEnvironment& contractEnvironment,
                                             ExecutorEnvironment& executorEnvironment)
        : m_contractEnvironment(contractEnvironment)
          , m_executorEnvironment(executorEnvironment)
          , m_nextBatchIndex(nextBatchIndex) {}

void DefaultBatchesManager::addManualCall(const CallRequestParameters& request) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_batches.empty() || (--m_batches.end())->second.m_batchFormationStatus !=
                             DraftBatch::BatchFormationStatus::MANUAL) {
        m_batches[m_nextDraftBatchIndex++] = DraftBatch();
    }

    auto batchIt = --m_batches.end();

    batchIt->second.m_requests.emplace_back(request, vm::CallRequest::CallLevel::MANUAL);
}

void DefaultBatchesManager::setAutomaticExecutionsEnabledSince(const std::optional<uint64_t>& blockHeight) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_automaticExecutionsEnabledSince.has_value() == blockHeight.has_value()) {
        // TODO Can have value but differ?
        return;
    }

    m_automaticExecutionsEnabledSince = blockHeight;

    if (!m_automaticExecutionsEnabledSince) {
        for (auto&[_, batch]: m_batches) {
            if (batch.m_requests.back().m_callLevel == vm::CallRequest::CallLevel::AUTOMATIC) {
                ASSERT(batch.m_batchFormationStatus == DraftBatch::BatchFormationStatus::FINISHED, m_executorEnvironment.logger())
                batch.m_requests.pop_back();
            }
            else if (batch.m_batchFormationStatus == DraftBatch::BatchFormationStatus::AUTOMATIC) {
                batch.m_batchFormationStatus = DraftBatch::BatchFormationStatus::FINISHED;
            }
        }
        m_autorunCallInfos.clear();
    }
}

bool DefaultBatchesManager::hasNextBatch() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    clearOutdatedBatches();

    if (m_delayedBatch) {
        return true;
    }

    return !m_batches.empty() &&
           m_batches.begin()->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::FINISHED;
}

Batch DefaultBatchesManager::nextBatch() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    clearOutdatedBatches();

    if (m_delayedBatch) {
        auto batch = std::move(*m_delayedBatch);
        m_delayedBatch.reset();
        return batch;
    }

    auto batch = std::move(m_batches.extract(m_batches.begin()).mapped());

    return Batch{m_nextBatchIndex++, std::move(batch.m_requests)};
}

void DefaultBatchesManager::addBlockInfo(const Block& block) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (!m_automaticExecutionsEnabledSince || block.m_height < *m_automaticExecutionsEnabledSince) {
        // Automatic Executions Are Disabled For This Block

        if (m_batches.empty()) {
            return;
        }

        auto batchIt = --m_batches.end();

        if (batchIt->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::MANUAL) {
            batchIt->second.m_batchFormationStatus = DraftBatch::BatchFormationStatus::FINISHED;
        }
    } else {
        if (m_batches.empty() || (--m_batches.end())->second.m_batchFormationStatus !=
                                 DraftBatch::BatchFormationStatus::MANUAL) {
            m_batches[m_nextDraftBatchIndex++] = DraftBatch();
        }

        auto batchIt = --m_batches.end();
        batchIt->second.m_batchFormationStatus = DraftBatch::BatchFormationStatus::AUTOMATIC;

        CallId callId;

        std::seed_seq seed(block.m_blockHash.begin(), block.m_blockHash.end());
        std::mt19937 rng(seed);
        std::generate_n(callId.begin(), sizeof(CallId), rng);

        auto callManager = runAutorunCall(callId, block.m_height);

        m_autorunCallInfos[callId] = AutorunCallInfo{batchIt->first, block.m_height, block.m_blockHash, std::move(callManager), Timer()};
    }
}

std::unique_ptr<CallExecutionManager> DefaultBatchesManager::runAutorunCall(const CallId& callId, uint64_t height) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    vm::CallRequest request(CallRequestParameters{callId,
                                                  m_executorEnvironment.executorConfig().autorunFile(),
                                                  m_executorEnvironment.executorConfig().autorunFunction(),
                                                  {},
                                                  m_executorEnvironment.executorConfig().autorunSCLimit(),
                                                  0},
                            vm::CallRequest::CallLevel::AUTORUN);

    auto[query, callback] = createAsyncQuery<vm::CallExecutionResult>([callId = request.m_callId, this](auto&& result) {
        if (result) {
            onSuperContractCallExecuted(callId, std::move(*result));
        } else {
            onSuperContractCallFailed(callId, std::move(result.error()));
        }
    }, [] {}, m_executorEnvironment, true, true);

    auto callManager = std::make_unique<CallExecutionManager>(
            m_executorEnvironment,
            std::make_shared<vm::VirtualMachineInternetQueryHandler>(),
            std::make_shared<AutorunBlockchainQueryHandler>(m_executorEnvironment, m_contractEnvironment, height),
            std::make_shared<vm::VirtualMachineStorageQueryHandler>(),
            std::move(query));

    callManager->run(request, std::move(callback));

    return callManager;
}

void DefaultBatchesManager::onSuperContractCallExecuted(const CallId& callId,
                                                        vm::CallExecutionResult&& executionResult) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto callIt = m_autorunCallInfos.find(callId);

    ASSERT(callIt != m_autorunCallInfos.end(), m_executorEnvironment.logger())

    ASSERT(callIt->second.m_callExecutionManager, m_executorEnvironment.logger())

    auto batchIt = m_batches.find(callIt->second.m_batchIndex);

    ASSERT(batchIt != m_batches.end(), m_executorEnvironment.logger())
    ASSERT(batchIt->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::AUTOMATIC,
           m_executorEnvironment.logger())

    if (executionResult.m_success && executionResult.m_return == 0) {
        CallReferenceInfo info;
        info.m_callerKey = CallerKey();
        batchIt->second.m_requests.emplace_back(CallRequestParameters{
                CallId{callIt->second.m_blockHash.array()},
                m_executorEnvironment.executorConfig().automaticExecutionFile(),
                m_executorEnvironment.executorConfig().automaticExecutionFunction(),
                {},
                m_contractEnvironment.automaticExecutionsSCLimit(),
                m_contractEnvironment.automaticExecutionsSMLimit(),
        }, vm::CallRequest::CallLevel::AUTOMATIC);
    }

    batchIt->second.m_batchFormationStatus = DraftBatch::BatchFormationStatus::FINISHED;

    if (batchIt->second.m_requests.empty()) {
        m_batches.erase(batchIt);
    }

    m_autorunCallInfos.erase(callIt);
}

void DefaultBatchesManager::onSuperContractCallFailed(const CallId& callId, std::error_code&& ec) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(ec == vm::ExecutionError::virtual_machine_unavailable, m_executorEnvironment.logger())

    auto callIt = m_autorunCallInfos.find(callId);

    ASSERT(callIt != m_autorunCallInfos.end(), m_executorEnvironment.logger())

    ASSERT(callIt->second.m_callExecutionManager, m_executorEnvironment.logger())

    callIt->second.m_callExecutionManager.reset();
    callIt->second.m_repeatTimer.cancel();

    callIt->second.m_repeatTimer = Timer(m_executorEnvironment.threadManager().context(),
                                         m_executorEnvironment.executorConfig().serviceUnavailableTimeoutMs(),
                                         [callId = callIt->first, this] {
                                             auto callIt = m_autorunCallInfos.find(callId);
                                             ASSERT(callIt != m_autorunCallInfos.end(), m_executorEnvironment.logger())
                                             callIt->second.m_callExecutionManager = runAutorunCall(callId, callIt->second.m_blockHeight);
                                         });
}

void DefaultBatchesManager::cancelBatchesTill(uint64_t batchIndex) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_storageSynchronizedBatchIndex = batchIndex;
}

void DefaultBatchesManager::clearOutdatedBatches() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_delayedBatch && m_delayedBatch->m_batchIndex <= m_storageSynchronizedBatchIndex) {
        // Note that we do NOT increment next batch index
        m_delayedBatch.reset();
    }

    while (!m_batches.empty() &&
           m_batches.begin()->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::FINISHED &&
           m_nextBatchIndex <= m_storageSynchronizedBatchIndex) {
        m_batches.erase(m_batches.begin());
        m_nextBatchIndex++;
    }
}

void DefaultBatchesManager::delayBatch(Batch&& batch) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_delayedBatch, m_executorEnvironment.logger())

    ASSERT(batch.m_batchIndex < m_nextBatchIndex, m_executorEnvironment.logger())

    m_delayedBatch = std::move(batch);
}

}