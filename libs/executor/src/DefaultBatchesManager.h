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

        DraftBatch(uint64_t blockHeight): m_blockHeight(blockHeight) {}

        // This implementation of batches manager assumes
        // that all the calls in the same batch are from the same block
        uint64_t m_blockHeight;

        BatchFormationStatus m_batchFormationStatus = BatchFormationStatus::MANUAL;
        std::deque<vm::CallRequest> m_requests;
    };

    struct AutorunCallInfo {
        uint64_t m_batchIndex;
        CallId m_callId;
        BlockHash m_blockHash;
        std::unique_ptr<CallExecutionManager> m_callExecutionManager;
        Timer m_repeatTimer;
    };

    ContractEnvironment& m_contractEnvironment;
    ExecutorEnvironment& m_executorEnvironment;

    uint64_t m_nextBatchIndex;

    uint64_t m_storageSynchronizedBatchIndex = 0;

    uint64_t m_nextDraftBatchIndex = 0;

    std::optional<uint64_t> m_automaticExecutionsEnabledSince;
	uint64_t m_unmodifiableUpTo = 0;

    std::map<uint64_t, AutorunCallInfo> m_autorunCallInfos;

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

    void addBlockInfo(uint64_t blockHeight, const blockchain::Block& block) override;

    void delayBatch(Batch&& batch) override;

	void setUnmodifiableUpTo(uint64_t blockHeight) override;

private:

    void onSuperContractCallExecuted(uint64_t blockHeight, vm::CallExecutionResult&& executionResult);

    void onSuperContractCallFailed(uint64_t blockHeight, std::error_code&& ec);

    std::unique_ptr<CallExecutionManager> runAutorunCall(const CallId& callId, uint64_t blockHeight);

    void cancelBatchesTill(uint64_t batchIndex) override;

    void clearOutdatedBatches();

    // Exclusive
    void disableAutomaticExecutionsTill(uint64_t batchIndex);

};

}