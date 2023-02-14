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
        std::deque<std::shared_ptr<CallRequest>> m_requests;
    };

    struct AutorunCallInfo {
        CallId m_callId;
        std::unique_ptr<CallExecutionManager> m_callExecutionManager;
        Timer m_repeatTimer;
    };

    ContractEnvironment& m_contractEnvironment;
    ExecutorEnvironment& m_executorEnvironment;

    uint64_t m_nextBatchIndex;

    uint64_t m_skippedNextBatchIndex = 0;

    std::optional<uint64_t> m_automaticExecutionsEnabledSince;
	uint64_t m_nextModifiableBlock = 0;

    std::map<uint64_t, AutorunCallInfo> m_autorunCallInfos;

    std::optional<Batch> m_delayedBatch;
    std::map<uint64_t, DraftBatch> m_batches;

    bool m_run = false;

public:

    DefaultBatchesManager(uint64_t nextBatchIndex,
                          ContractEnvironment& contractEnvironment,
                          ExecutorEnvironment& executorEnvironment);

public:

    void run() override;

    void addManualCall(const ManualCallRequest& request) override;

    void setAutomaticExecutionsEnabledSince(const std::optional<uint64_t>& blockHeight) override;

    bool hasNextBatch() override;

    Batch nextBatch() override;

    void addBlock(uint64_t blockHeight) override;

    void delayBatch(Batch&& batch) override;

	void fixUnmodifiable(uint64_t nextBlockHeight) override;

    bool isBatchValid(const Batch& batch) override;

    uint64_t minBatchIndex() override;

private:

    void onSuperContractCallExecuted(uint64_t blockHeight, vm::CallExecutionResult&& executionResult);

    void onSuperContractCallFailed(uint64_t blockHeight, std::error_code&& ec);

    std::unique_ptr<CallExecutionManager> runAutorunCall(const CallId& callId, uint64_t blockHeight);

    void skipBatches(uint64_t nextBatchIndex) override;

    void clearOutdatedBatches();

    void disableAutomaticExecutionsTill(uint64_t nextBatchIndex);

    void delayedBatchExtractAutomaticCall();
};

}