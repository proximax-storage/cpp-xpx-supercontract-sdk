/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "SynchronizationTask.h"

namespace sirius::contract {

SynchronizationTask::SynchronizationTask(SynchronizationRequest&& synchronizationRequest,
                                         ContractEnvironment& contractEnvironment,
                                         ExecutorEnvironment& executorEnvironment)
        : BaseContractTask(executorEnvironment, contractEnvironment)
        , m_request(std::move(synchronizationRequest)) {}

// region blockchain event handler

bool SynchronizationTask::onEndBatchExecutionPublished(const PublishedEndBatchExecutionTransactionInfo& info) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_contractEnvironment.addSynchronizationTask();

    terminate();

    return true;
}

bool SynchronizationTask::onStorageSynchronizedPublished(uint64_t) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    return false;
}

// endregion

void SynchronizationTask::run() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_storageQuery, m_executorEnvironment.logger())

    m_storageTimer.cancel();

    auto storage = m_executorEnvironment.storageModifier().lock();

    if (!storage) {
        onStorageUnavailable();
        return;
    }

    auto [query, callback] = createAsyncQuery<void>([this](auto&& res) {
        if (!res) {
            onStorageUnavailable();
            return;
        }
        onStorageStateSynchronized();
    }, [] {}, m_executorEnvironment, true, true);

    m_storageQuery = std::move(query);

    storage->synchronizeStorage(m_contractEnvironment.driveKey(), storageModificationId(m_request.m_batchIndex), m_request.m_storageHash, callback);
}

void SynchronizationTask::terminate() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_storageQuery.reset();
    m_storageTimer.cancel();

    m_contractEnvironment.finishTask();
}

void SynchronizationTask::onStorageUnavailable() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_storageQuery.reset();

    ASSERT(!m_storageTimer, m_executorEnvironment.logger())

    m_storageTimer = Timer(m_executorEnvironment.threadManager().context(), 5000, [this] {
        run();
    });
}

void SynchronizationTask::onStorageStateSynchronized() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.executorEventHandler().synchronizationSingleTransactionIsReady(
            SynchronizationSingleTransactionInfo{m_contractEnvironment.contractKey(), m_request.m_batchIndex});

    m_contractEnvironment.batchesManager().cancelBatchesTill(m_request.m_batchIndex);

    m_contractEnvironment.proofOfExecution().reset(m_request.m_batchIndex + 1);

    m_contractEnvironment.finishTask();
}

}
