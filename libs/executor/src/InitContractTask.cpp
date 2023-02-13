/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "InitContractTask.h"

namespace sirius::contract {

InitContractTask::InitContractTask(
        AddContractRequest&& request,
        std::shared_ptr<AsyncQueryCallback<void>>&& onTaskFinishedCallback,
        ContractEnvironment& contractEnvironment,
        ExecutorEnvironment& executorEnvironment)
        : BaseContractTask(executorEnvironment, contractEnvironment)
        , m_request(std::move(request))
        , m_onTaskFinishedCallback(std::move(onTaskFinishedCallback)) {}

void InitContractTask::run() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    for (const auto& [batchId, verificationInformation]: m_request.m_recentBatchesInformation) {
        m_contractEnvironment.proofOfExecution().addBatchVerificationInformation(batchId, verificationInformation);
    }

    if (m_request.m_recentBatchesInformation.empty()) {
        // No batches have already been executed
        // We must ensure that storage is in the actual state before trying to execute calls
        requestActualModificationId();
    }
    // Otherwise, we wait for published end batch execution
}


void InitContractTask::terminate() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_storageActualModificationIdQuery.reset();
    m_storageActualModificationIdTimer.cancel();

    m_onTaskFinishedCallback->postReply(expected<void>());
}

void InitContractTask::requestActualModificationId() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto storage = m_executorEnvironment.storage().lock();

    if (!storage) {
        onActualModificationIdReceived(
                tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[query, callback] = createAsyncQuery<ModificationId>([this](auto&& res) {
                                                                 onActualModificationIdReceived(std::move(res));
                                                             },
                                                             [] {},
                                                             m_executorEnvironment,
                                                             true,
                                                             true);
    m_storageActualModificationIdQuery = std::move(query);
    storage->actualModificationId(m_contractEnvironment.driveKey(), std::move(callback));
}

void InitContractTask::onActualModificationIdReceived(const expected<ModificationId>& res) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (!res || *res != m_request.m_contractDeploymentBaseModificationId) {
        m_storageActualModificationIdTimer = Timer(m_executorEnvironment.threadManager().context(),
                                                   m_executorEnvironment.executorConfig().serviceUnavailableTimeoutMs(),
                                                   [this] {
            requestActualModificationId();
        });
        return;
    }

    m_contractEnvironment.batchesManager().run();
    m_onTaskFinishedCallback->postReply(expected<void>());
}

// region blockchain event handler

bool InitContractTask::onEndBatchExecutionPublished(const PublishedEndBatchExecutionTransactionInfo& info) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_contractEnvironment.addSynchronizationTask();

    terminate();

    return true;
}

// endregion

}