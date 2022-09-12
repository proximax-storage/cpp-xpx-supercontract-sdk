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

void DefaultBatchesManager::addCall(const CallRequest& request) {
    if (m_batches.empty() || (--m_batches.end())->second.m_batchFormationStatus !=
                             DraftBatch::BatchFormationStatus::MANUAL) {
        m_batches[m_nextDraftBatchIndex++] = DraftBatch();
    }

    auto batchIt = --m_batches.end();

    batchIt->second.m_requests.push_back(request);
}

void DefaultBatchesManager::setAutomaticExecutionsEnabledSince(const std::optional<uint64_t>& blockHeight) {

    if (m_automaticExecutionsEnabledSince.has_value() == blockHeight.has_value()) {
        // TODO Can have value but differ?
        return;
    }

    m_automaticExecutionsEnabledSince = blockHeight;

    if (!m_automaticExecutionsEnabledSince) {
        for (auto&[_, batch]: m_batches) {
            if (batch.m_requests.back().m_callLevel == CallRequest::CallLevel::AUTOMATIC) {
                batch.m_requests.pop_back();
            }
        }
    }
}

bool DefaultBatchesManager::hasNextBatch() {
    clearOutdatedBatches();
    return !m_batches.empty() &&
           m_batches.begin()->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::FINISHED;
}

Batch DefaultBatchesManager::nextBatch() {
    clearOutdatedBatches();

    auto batch = std::move(m_batches.extract(m_batches.begin()).mapped());

    return Batch{m_nextBatchIndex++, std::move(batch.m_requests)};
}

void DefaultBatchesManager::addBlockInfo(const Block& block) {
    if (!m_automaticExecutionsEnabledSince || m_automaticExecutionsEnabledSince < block.m_height) {
        // Automatic Executions Are Disabled For This Block

        if ( m_batches.empty() ) {
            return;
        }

        auto batchIt = --m_batches.end();

        if ( batchIt->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::MANUAL ) {
            batchIt->second.m_batchFormationStatus = DraftBatch::BatchFormationStatus::FINISHED;
        }
    }
    else {
        if ( m_batches.empty() || (--m_batches.end())->second.m_batchFormationStatus !=
        DraftBatch::BatchFormationStatus::MANUAL ) {
            m_batches[m_nextDraftBatchIndex++] = DraftBatch();
        }

        auto batchIt = --m_batches.end();
        batchIt->second.m_batchFormationStatus = DraftBatch::BatchFormationStatus::AUTOMATIC;

        CallId callId;

        std::seed_seq seed(block.m_blockHash.begin(), block.m_blockHash.end());
        std::mt19937 rng(seed);
        std::generate_n(callId.begin(), sizeof(CallId), rng);

        m_autorunCallInfos[callId] = AutorunCallInfo{batchIt->first, block.m_blockHash};

        if ( auto pVirtualMachine = m_executorEnvironment.virtualMachine().lock(); pVirtualMachine ) {
            CallRequest request{m_contractEnvironment.contractKey(),
                                callId,
                                m_executorEnvironment.executorConfig().autorunFile(),
                                m_executorEnvironment.executorConfig().autorunFunction(),
                                {},
                                m_executorEnvironment.executorConfig().autorunSCLimit(),
                                0,
                                CallRequest::CallLevel::AUTORUN};

        }
    }
}

void DefaultBatchesManager::onSuperContractCallExecuted(const CallId& callId,
                                                        vm::CallExecutionResult&& executionResult) {

    auto callIt = m_autorunCallInfos.find(callId);

    if (callIt == m_autorunCallInfos.end()) {
        return;
    }

    auto batchIt = m_batches.find(callIt->second.m_batchIndex);

    ASSERT(batchIt != m_batches.end(), m_executorEnvironment.logger())
    ASSERT (batchIt->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::AUTOMATIC, m_executorEnvironment.logger())

    if (executionResult.m_success && executionResult.m_return == 0) {
        CallReferenceInfo info;
        info.m_callerKey = CallerKey();
        batchIt->second.m_requests.push_back(CallRequest{
                m_contractEnvironment.contractKey(),
                CallId{callIt->second.m_blockHash.array()},
                m_executorEnvironment.executorConfig().automaticExecutionFile(),
                m_executorEnvironment.executorConfig().automaticExecutionFunction(),
                {},
                m_contractEnvironment.automaticExecutionsSCLimit(),
                m_contractEnvironment.automaticExecutionsSMLimit(),
                CallRequest::CallLevel::AUTOMATIC
        });
    }

    batchIt->second.m_batchFormationStatus = DraftBatch::BatchFormationStatus::FINISHED;

    if (batchIt->second.m_requests.empty()) {
        m_batches.erase(batchIt);
    }

    m_autorunCallInfos.erase(callIt);
}

}