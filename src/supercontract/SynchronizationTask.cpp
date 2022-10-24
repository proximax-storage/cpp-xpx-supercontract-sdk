/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "SynchronizationTask.h"

// TODO The class is not fully implemented

namespace sirius::contract {

SynchronizationTask::SynchronizationTask(SynchronizationRequest&& synchronizationRequest,
                                         ContractEnvironment& contractEnvironment,
                                         ExecutorEnvironment& executorEnvironment)
        : BaseContractTask(executorEnvironment, contractEnvironment)
        , m_request(std::move(synchronizationRequest)) {}

// region blockchain event handler

bool SynchronizationTask::onEndBatchExecutionPublished(const PublishedEndBatchExecutionTransactionInfo& info) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    const auto& cosigners = info.m_cosigners;

    if (std::find(cosigners.begin(), cosigners.end(), m_executorEnvironment.keyPair().publicKey()) ==
        cosigners.end()) {
        m_request = {info.m_driveState};

        run();
    }

    // TODO What if we are among the cosigners? Is it possible?

    return true;
}

bool SynchronizationTask::onStorageSynchronized(uint64_t) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_contractEnvironment.finishTask();

    return true;
}

// endregion

void SynchronizationTask::run() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_storageQuery, m_executorEnvironment.logger());

    auto storage = m_executorEnvironment.storage().lock();

    if (!storage) {

    }

    auto [query, callback] = createAsyncQuery<bool>([this](auto&& res) {

        if (!res) {
            onStorageUnavailable();
            return;
        }

        onStorageStateSynchronized();
    }, [] {}, m_executorEnvironment, true, true);

    storage->synchronizeStorage(m_contractEnvironment.driveKey(), m_request.m_storageHash, callback);
}

void SynchronizationTask::terminate() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_contractEnvironment.finishTask();
}

void SynchronizationTask::onStorageUnavailable() {

    m_storageQuery.reset();

    m_storageTimer = Timer(m_executorEnvironment.threadManager().context(), 5000, [this] {
        run();
    });
}

void SynchronizationTask::onStorageStateSynchronized() {

}

}
