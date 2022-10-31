/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "DefaultBatchesManager.h"

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
                batch.m_requests.pop_back();
            }
        }
    }
}

bool DefaultBatchesManager::hasNextBatch() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    clearOutdatedBatches();
    return !m_batches.empty() &&
           m_batches.begin()->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::FINISHED;
}

Batch DefaultBatchesManager::nextBatch() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    clearOutdatedBatches();

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

        m_autorunCallInfos[callId] = AutorunCallInfo{batchIt->first, block.m_blockHash};
        vm::CallRequest request(CallRequestParameters{m_contractEnvironment.contractKey(),
                                                      callId,
                                                      m_executorEnvironment.executorConfig().autorunFile(),
                                                      m_executorEnvironment.executorConfig().autorunFunction(),
                                                      {},
                                                      m_executorEnvironment.executorConfig().autorunSCLimit(),
                                                      0},
                                vm::CallRequest::CallLevel::AUTORUN);

        auto[query, callback] = createAsyncQuery<vm::CallExecutionResult>([=, this](auto&& result) {
            ASSERT(result, m_executorEnvironment.logger())
            onSuperContractCallExecuted(request.m_callId, std::move(*result));
        }, [] {}, m_executorEnvironment, false, false);

        auto manager = std::make_unique<CallExecutionManager>(m_executorEnvironment,
                                                              m_executorEnvironment.virtualMachine(),
                                                              m_executorEnvironment.executorConfig().virtualMachineRepeatTimeoutMs(),
                                                              request,
                                                              nullptr,
                                                              nullptr,
                                                              callback);

        m_autorunCallInfos[callId] = AutorunCallInfo{batchIt->first, block.m_blockHash, std::move(manager)};
    }
}

void DefaultBatchesManager::onSuperContractCallExecuted(const CallId& callId,
                                                        vm::CallExecutionResult&& executionResult) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto callIt = m_autorunCallInfos.find(callId);

    if (callIt == m_autorunCallInfos.end()) {
        return;
    }

    auto batchIt = m_batches.find(callIt->second.m_batchIndex);

    ASSERT(batchIt != m_batches.end(), m_executorEnvironment.logger())
    ASSERT(batchIt->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::AUTOMATIC,
           m_executorEnvironment.logger())

    if (executionResult.m_success && executionResult.m_return == 0) {
        CallReferenceInfo info;
        info.m_callerKey = CallerKey();
        batchIt->second.m_requests.emplace_back(CallRequestParameters{
                m_contractEnvironment.contractKey(),
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

bool DefaultBatchesManager::onStorageSynchronized(uint64_t batchIndex) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_storageSynchronizedBatchIndex = batchIndex;

    return true;
}

void DefaultBatchesManager::clearOutdatedBatches() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    while (!m_batches.empty() &&
           m_batches.begin()->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::FINISHED &&
           m_nextBatchIndex <= m_storageSynchronizedBatchIndex) {
        m_batches.erase(m_batches.begin());
        m_nextBatchIndex++;
    }
}

}