/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BaseContractTask.h"
#include "CallExecutionManager.h"
#include "BatchesManager.h"
#include "CallExecutionEnvironment.h"

namespace sirius::contract {

class BatchExecutionTask
        : public BaseContractTask {

private:

    const Batch m_batch;
    decltype(m_batch.m_callRequests.begin()) m_callIterator;

    std::vector<SuccessfulCallExecutionOpinion> m_callsExecutionOpinions;

    std::unique_ptr<CallExecutionManager> m_callExecutionManager;

    std::shared_ptr<AsyncQuery> m_storageQuery;

    std::optional<SuccessfulEndBatchExecutionOpinion> m_successfulEndBatchOpinion;
    std::optional<UnsuccessfulEndBatchExecutionOpinion> m_unsuccessfulEndBatchOpinion;

    std::optional<PublishedEndBatchExecutionTransactionInfo> m_publishedEndBatchInfo;

    std::map<ExecutorKey, SuccessfulEndBatchExecutionOpinion> m_otherSuccessfulExecutorEndBatchOpinions;
    std::map<ExecutorKey, UnsuccessfulEndBatchExecutionOpinion> m_otherUnsuccessfulExecutorEndBatchOpinions;

    Timer m_unsuccessfulExecutionTimer;

    Timer m_successfulApprovalExpectationTimer;
    Timer m_unsuccessfulApprovalExpectationTimer;

    Timer m_unableToExecuteBatchTimer;

    Timer m_shareOpinionTimer;

    bool m_successfulEndBatchSent = false;
    bool m_unsuccessfulEndBatchSent = false;

    uint64_t m_proofOfExecutionSecretData = 0;

    bool     m_finished = false;

public:

    BatchExecutionTask(Batch&& batch,
                       ContractEnvironment& contractEnvironment,
                       ExecutorEnvironment& executorEnvironment,
                       std::map<ExecutorKey, SuccessfulEndBatchExecutionOpinion>&& otherSuccessfulExecutorEndBatchOpinions,
                       std::map<ExecutorKey, UnsuccessfulEndBatchExecutionOpinion>&& otherUnsuccessfulExecutorEndBatchOpinions,
                       std::optional<PublishedEndBatchExecutionTransactionInfo>&& publishedEndBatchInfo);

    void terminate() override;

    void run() override;

private:

    void onSuperContractCallExecuted(const CallId& callId, vm::CallExecutionResult&& executionResult);

public:

    // region message event handler

    bool onEndBatchExecutionOpinionReceived(const SuccessfulEndBatchExecutionOpinion& opinion) override;

    // endregion

private:

    void onInitiatedStorageModifications();

    void onInitiatedSandboxModification(vm::CallRequest&& callRequest);

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

    bool validateOtherBatchInfo(const SuccessfulEndBatchExecutionOpinion& other);

    void checkEndBatchTransactionReadiness();

    EndBatchExecutionTransactionInfo
    createMultisigTransactionInfo(const SuccessfulEndBatchExecutionOpinion& transactionOpinion,
                                  std::map<ExecutorKey, SuccessfulEndBatchExecutionOpinion>&& otherTransactionOpinions);

    void sendEndBatchTransaction(const EndBatchExecutionTransactionInfo& transactionInfo);

    void executeNextCall();

    void onUnsuccessfulExecutionTimerExpiration();

    void computeProofOfExecution();

    void onUnableToExecuteBatch();

    void onAppliedStorageModifications();

    void shareOpinions();
};

}