/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <random>

#include "BatchesManager.h"

#include "ExecutorEnvironment.h"
#include "ContractEnvironment.h"

#include "log.h"

namespace sirius::contract {

class DefaultBatchesManager
        : public BaseBatchesManager {

private:

    struct DraftBatch {

        enum class BatchFormationStatus {
            MANUAL, AUTOMATIC, FINISHED
        };

        BatchFormationStatus m_batchFormationStatus = BatchFormationStatus::MANUAL;
        std::deque<CallRequest> m_requests;
    };

    struct AutorunCallInfo {
        uint64_t m_batchIndex;
        BlockHash m_blockHash;
        std::shared_ptr<AsyncQuery> m_query;
    };

    ContractEnvironment& m_contractEnvironment;
    ExecutorEnvironment& m_executorEnvironment;

    uint64_t m_nextBatchIndex;

    uint64_t m_storageSynchronizedBatchIndex = 0;

    uint64_t m_nextDraftBatchIndex = 0;

    std::optional<uint64_t> m_automaticExecutionsEnabledSince;

    std::map<CallId, AutorunCallInfo> m_autorunCallInfos;

    std::map<uint64_t, DraftBatch> m_batches;

public:

    DefaultBatchesManager(uint64_t nextBatchIndex,
                          ContractEnvironment& contractEnvironment,
                          ExecutorEnvironment& executorEnvironment);

public:

    void addCall(const CallRequest& request) override;

    void setAutomaticExecutionsEnabledSince(const std::optional<uint64_t>& blockHeight) override;

    bool hasNextBatch() override;

    Batch nextBatch() override;

    void addBlockInfo(const Block& block) override;

private:

    void onSuperContractCallExecuted(const CallId& callId, vm::CallExecutionResult&& executionResult);

public:

    // region blockchain event handler

    bool onStorageSynchronized(uint64_t batchIndex) override {
        m_storageSynchronizedBatchIndex = batchIndex;

        return true;
    }

    // endregion

private:

    void clearOutdatedBatches() {
        while (!m_batches.empty() &&
               m_batches.begin()->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::FINISHED &&
               m_nextBatchIndex <= m_storageSynchronizedBatchIndex) {
            m_batches.erase(m_batches.begin());
            m_nextBatchIndex++;
        }
    }

};

}