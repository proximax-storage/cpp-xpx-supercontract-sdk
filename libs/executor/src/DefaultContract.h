/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "Contract.h"
#include "ContractEnvironment.h"
#include "ProofOfExecution.h"
#include "BatchesManager.h"
#include "BaseContractTask.h"
#include "supercontract/Identifiers.h"

namespace sirius::contract {

class DefaultContract
        : private SingleThread
        , public Contract
        , public ContractEnvironment {

private:

    ContractKey m_contractKey;

    DriveKey m_driveKey;
    std::map<ExecutorKey, ExecutorInfo> m_executors;

    uint64_t m_automaticExecutionsSCLimit;
    uint64_t m_automaticExecutionsSMLimit;

    ExecutorEnvironment& m_executorEnvironment;
    ContractConfig m_contractConfig;

    ProofOfExecution m_proofOfExecution;

    SynchronizationRequest m_lastKnownStorageState;

    // Requests
    std::unique_ptr<BaseBatchesManager> m_batchesManager;
    std::optional<SynchronizationRequest> m_synchronizationRequest;
    std::optional<RemoveRequest> m_contractRemoveRequest;

    std::unique_ptr<BaseContractTask> m_task;

    std::map<uint64_t, std::map<ExecutorKey, SuccessfulEndBatchExecutionOpinion>> m_unknownSuccessfulBatchOpinions;
    std::map<uint64_t, std::map<ExecutorKey, UnsuccessfulEndBatchExecutionOpinion>> m_unknownUnsuccessfulBatchOpinions;
    std::map<uint64_t, PublishedEndBatchExecutionTransactionInfo> m_unknownPublishedEndBatchTransactions;

public:

    DefaultContract(const ContractKey& contractKey,
                    AddContractRequest&& addContractRequest,
                    ExecutorEnvironment& contractContext);

    void terminate() override;

    void addManualCall(const CallRequestParameters& request) override;

    void removeContract(const RemoveRequest& request) override;

    void setExecutors(std::map<ExecutorKey, ExecutorInfo>&& executors) override;

    void addBlockInfo(const Block& block) override;

    void setAutomaticExecutionsEnabledSince(const std::optional<uint64_t>& blockHeight) override;

public:

    // region blockchain event handler

    bool onEndBatchExecutionPublished(const PublishedEndBatchExecutionTransactionInfo& info) override;

    bool onEndBatchExecutionFailed(const FailedEndBatchExecutionTransactionInfo& info) override;

    bool onStorageSynchronizedPublished(uint64_t batchIndex) override;

    // endregion

public:

    // region message event handler

    bool onEndBatchExecutionOpinionReceived(const SuccessfulEndBatchExecutionOpinion& info) override;

    bool onEndBatchExecutionOpinionReceived(const UnsuccessfulEndBatchExecutionOpinion& info) override;

    // endregion

public:

    // region task context

    const ContractKey& contractKey() const override;

    const std::map<ExecutorKey, ExecutorInfo>& executors() const override;

    const DriveKey& driveKey() const override;

    uint64_t automaticExecutionsSCLimit() const override;

    uint64_t automaticExecutionsSMLimit() const override;

    const ContractConfig& contractConfig() const override;

    void finishTask() override;

    void addSynchronizationTask() override;

    void delayBatchExecution(Batch batch) override;

    void cancelBatchesUpTo(uint64_t index) override;

    ProofOfExecution& proofOfExecution() override;

    // endregion

private:

    void runTask();

    void runInitializeContractTask(AddContractRequest&& request);

    void runRemoveContractTask();

    void runSynchronizationTask();

    void runBatchExecutionTask();

};

}