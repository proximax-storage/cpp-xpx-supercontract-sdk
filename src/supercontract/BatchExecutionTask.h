/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BaseContractTask.h"
#include "CallExecutionManager.h"
#include "BatchesManager.h"

namespace sirius::contract {

class BatchExecutionTask
        : public BaseContractTask {

private:

    Batch m_batch;

    std::vector<CallExecutionOpinion> m_callsExecutionOpinions;

    std::unique_ptr<CallExecutionManager> m_callManager;

    std::shared_ptr<AsyncQuery> m_storageQuery;

    std::optional<EndBatchExecutionOpinion> m_successfulEndBatchOpinion;
    std::optional<EndBatchExecutionOpinion> m_unsuccessfulEndBatchOpinion;

    std::optional<PublishedEndBatchExecutionTransactionInfo> m_publishedEndBatchInfo;

    std::map<ExecutorKey, EndBatchExecutionOpinion> m_otherSuccessfulExecutorEndBatchOpinions;
    std::map<ExecutorKey, EndBatchExecutionOpinion> m_otherUnsuccessfulExecutorEndBatchOpinions;

    Timer m_unsuccessfulExecutionTimer;

    Timer m_successfulApprovalExpectationTimer;
    Timer m_unsuccessfulApprovalExpectationTimer;

    bool m_successfulEndBatchSent = false;
    bool m_unsuccessfulEndBatchSent = false;

public:

    BatchExecutionTask(Batch&& batch,
                       ContractEnvironment& contractEnvironment,
                       ExecutorEnvironment& executorEnvironment,
                       std::map<ExecutorKey, EndBatchExecutionOpinion>&& otherSuccessfulExecutorEndBatchOpinions,
                       std::map<ExecutorKey, EndBatchExecutionOpinion>&& otherUnsuccessfulExecutorEndBatchOpinions,
                       std::optional<PublishedEndBatchExecutionTransactionInfo>&& publishedEndBatchInfo);

    void terminate() override;

    void run() override;

private:

    void onSuperContractCallExecuted(const CallId& callId, vm::CallExecutionResult&& executionResult);

public:

    // region message event handler

    bool onEndBatchExecutionOpinionReceived(const EndBatchExecutionOpinion& opinion) override;

    // endregion

public:

    void onInitiatedModifications();

    void onAppliedSandboxStorageModifications(const CallId& callId,
                                              vm::CallExecutionResult&& executionResult,
                                              storage::SandboxModificationDigest&& digest);

    void onStorageHashEvaluated(storage::StorageState&& storageState);

public:

    // region blockchain event handler

    bool onEndBatchExecutionPublished(const PublishedEndBatchExecutionTransactionInfo& info) override;

    bool onEndBatchExecutionFailed(const FailedEndBatchExecutionTransactionInfo& info) override;

    // endregion

private:

    void formSuccessfulEndBatchOpinion(const StorageHash& storageHash,
                                       uint64_t usedDriveSize, uint64_t metaFilesSize, uint64_t fileStructureSize);

    void processPublishedEndBatch();

    bool validateOtherBatchInfo(const EndBatchExecutionOpinion& other);

    void checkEndBatchTransactionReadiness();

    EndBatchExecutionTransactionInfo
    createMultisigTransactionInfo(const EndBatchExecutionOpinion& transactionOpinion,
                                  std::map<ExecutorKey, EndBatchExecutionOpinion>&& otherTransactionOpinions);

    void sendEndBatchTransaction(const EndBatchExecutionTransactionInfo& transactionInfo);

    void executeNextCall();

    void onUnsuccessfulExecutionTimerExpiration();

    void computeProofOfExecution();

    void onStorageUnavailable();

    void onAppliedStorageModifications();
};

}