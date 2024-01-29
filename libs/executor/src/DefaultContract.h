/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "Contract.h"
#include "ContractEnvironment.h"
#include "ProofOfExecution.h"
#include "BatchesManager.h"
#include "BaseContractTask.h"
#include <common/Identifiers.h>

namespace sirius::contract {

class DefaultContract
        : private SingleThread
        , public Contract
        , public ContractEnvironment {

private:

    struct ExtendedRemoveRequest {
        RemoveRequest m_request;
        std::shared_ptr<AsyncQueryCallback<void>> m_callback;
    };

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
    bool m_hasSynchronizationRequest = false;
    std::optional<ExtendedRemoveRequest> m_contractRemoveRequest;

    std::unique_ptr<BaseContractTask> m_task;

    std::map<uint64_t, std::map<ExecutorKey, SuccessfulEndBatchExecutionOpinion>> m_unknownSuccessfulBatchOpinions;
    std::map<uint64_t, std::map<ExecutorKey, UnsuccessfulEndBatchExecutionOpinion>> m_unknownUnsuccessfulBatchOpinions;
    std::map<uint64_t, blockchain::PublishedEndBatchExecutionTransactionInfo> m_unknownPublishedEndBatchTransactions;

public:

    DefaultContract(const ContractKey& contractKey,
                    AddContractRequest&& addContractRequest,
                    ExecutorEnvironment& contractContext);

    void terminate() override;

    void addManualCall(const ManualCallRequest& request) override;

    void removeContract(const RemoveRequest& request, std::shared_ptr<AsyncQueryCallback<void>>&& callback) override;

    void setExecutors(std::map<ExecutorKey, ExecutorInfo>&& executors) override;

    void setAutomaticExecutionsEnabledSince(uint64_t blockHeight) override;

public:

    // region blockchain event handler

    bool onEndBatchExecutionPublished(const blockchain::PublishedEndBatchExecutionTransactionInfo& info) override;

    bool onEndBatchExecutionFailed(const blockchain::FailedEndBatchExecutionTransactionInfo& info) override;

    bool onStorageSynchronizedPublished(uint64_t batchIndex) override;

    bool onBlockPublished(uint64_t blockHeight) override;

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

    void addSynchronizationTask() override;

    void notifyHasNextBatch() override;

    BaseBatchesManager& batchesManager() override;

    ProofOfExecution& proofOfExecution() override;

    // endregion

private:

    void runTask();

    void runInitializeContractTask(AddContractRequest&& request);

    void runRemoveContractTask();

    void runSynchronizationTask();

    void runBatchExecutionTask();

private:

    template<class TCacheEntry>
    void clearOutdatedCache(std::map<uint64_t, TCacheEntry>& cache, uint64_t to) {
        while (!cache.empty() && cache.begin()->first < to) {
            cache.erase(cache.begin());
        }
    }

};

}