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

    std::multimap<Hash256, blockchain::SerializedAggregatedTransaction> m_releasedTransactions;

    Timer m_unsuccessfulExecutionTimer;

    Timer m_successfulApprovalExpectationTimer;
    Timer m_unsuccessfulApprovalExpectationTimer;

    Timer m_unableToExecuteBatchTimer;

    Timer m_shareOpinionTimer;

    bool m_successfulEndBatchSent = false;
    bool m_unsuccessfulEndBatchSent = false;

    uint64_t m_proofOfExecutionSecretData = 0;

    bool     m_finished = false;

    std::shared_ptr<AsyncQueryCallback<void>> m_onTaskFinishedCallback;

public:

    BatchExecutionTask(Batch&& batch,
                       std::shared_ptr<AsyncQueryCallback<void>>&& onTaskFinishedCallback,
                       ContractEnvironment& contractEnvironment,
                       ExecutorEnvironment& executorEnvironment);

    void terminate() override;

    void run() override;

private:

    void onSuperContractCallExecuted(std::shared_ptr<CallRequest>&& callRequest,
                                     vm::CallExecutionResult&& executionResult);

public:

    // region message event handler

    bool onEndBatchExecutionOpinionReceived(const SuccessfulEndBatchExecutionOpinion& opinion) override;

    bool onEndBatchExecutionOpinionReceived(const UnsuccessfulEndBatchExecutionOpinion& opinion) override;

    // endregion

private:

    void onInitiatedStorageModifications();

    void onInitiatedSandboxModification(std::shared_ptr<CallRequest>&& callRequest);

    void onAppliedSandboxStorageModifications(std::shared_ptr<CallRequest>&& callRequest,
                                              vm::CallExecutionResult&& executionResult,
                                              storage::SandboxModificationDigest&& digest);

    void onStorageHashEvaluated(storage::StorageState&& storageState);

public:

    // region blockchain event handler

    bool onEndBatchExecutionPublished(const PublishedEndBatchExecutionTransactionInfo& info) override;

    bool onEndBatchExecutionFailed(const FailedEndBatchExecutionTransactionInfo& info) override;

    bool onBlockPublished(uint64_t height) override;

    // endregion

private:

    void formSuccessfulEndBatchOpinion(const StorageHash& storageHash,
                                       uint64_t usedDriveSize, uint64_t metaFilesSize, uint64_t fileStructureSize);

    void processPublishedEndBatch();

    bool validateOtherBatchInfo(const SuccessfulEndBatchExecutionOpinion& other);

    bool validateOtherBatchInfo(const UnsuccessfulEndBatchExecutionOpinion& other);

    void checkEndBatchTransactionReadiness();

    blockchain::SuccessfulEndBatchExecutionTransactionInfo
    createMultisigTransactionInfo(const SuccessfulEndBatchExecutionOpinion& transactionOpinion,
                                  std::map<ExecutorKey, SuccessfulEndBatchExecutionOpinion> otherTransactionOpinions);

    blockchain::UnsuccessfulEndBatchExecutionTransactionInfo
    createMultisigTransactionInfo(const UnsuccessfulEndBatchExecutionOpinion& transactionOpinion,
                                  std::map<ExecutorKey, UnsuccessfulEndBatchExecutionOpinion> otherTransactionOpinions);

    void sendEndBatchTransaction(const blockchain::SuccessfulEndBatchExecutionTransactionInfo& transactionInfo);

    void sendEndBatchTransaction(const blockchain::UnsuccessfulEndBatchExecutionTransactionInfo& transactionInfo);

    void executeNextCall();

    void onUnsuccessfulExecutionTimerExpiration();

    void computeProofOfExecution();

    void onUnableToExecuteBatch(const std::error_code& ec);

    void onAppliedStorageModifications();

    void shareOpinions();
};

}