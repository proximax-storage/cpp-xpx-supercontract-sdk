/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <random>

#include "BatchesManager.h"

#include "ExecutorEnvironment.h"
#include "ContractEnvironment.h"

#include "CallExecutionManager.h"
#include <supercontract/SingleThread.h>

namespace sirius::contract {

class DefaultBatchesManager
        : public BaseBatchesManager, private SingleThread {

private:

    struct DraftBatch {

        enum class BatchFormationStatus {
            MANUAL, AUTOMATIC, FINISHED
        };

        BatchFormationStatus m_batchFormationStatus = BatchFormationStatus::MANUAL;
        std::deque<vm::CallRequest> m_requests;
    };

    struct AutorunCallInfo {
        uint64_t m_batchIndex;
        BlockHash m_blockHash;
        std::unique_ptr<CallExecutionManager> m_manager;
    };

    ContractEnvironment& m_contractEnvironment;
    ExecutorEnvironment& m_executorEnvironment;

    uint64_t m_nextBatchIndex;

    uint64_t m_storageSynchronizedBatchIndex = 0;

    uint64_t m_nextDraftBatchIndex = 0;

    std::optional<uint64_t> m_automaticExecutionsEnabledSince;

    std::map<CallId, AutorunCallInfo> m_autorunCallInfos;

    std::optional<Batch> m_delayedBatch;
    std::map<uint64_t, DraftBatch> m_batches;

public:

    DefaultBatchesManager(uint64_t nextBatchIndex,
                          ContractEnvironment& contractEnvironment,
                          ExecutorEnvironment& executorEnvironment);

public:

    void addManualCall(const CallRequestParameters& request) override;

    void setAutomaticExecutionsEnabledSince(const std::optional<uint64_t>& blockHeight) override;

    bool hasNextBatch() override;

    Batch nextBatch() override;

    void addBlockInfo(const Block& block) override;

    void delayBatch(Batch&& batch) override;

private:

    void onSuperContractCallExecuted(const CallId& callId, vm::CallExecutionResult&& executionResult);

public:

    // region blockchain event handler

    bool onStorageSynchronized(uint64_t batchIndex) override;

    // endregion

private:

    void clearOutdatedBatches();

};

}