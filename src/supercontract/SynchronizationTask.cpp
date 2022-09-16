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

    const auto& cosigners = info.m_cosigners;

    if (std::find(cosigners.begin(), cosigners.end(), m_executorEnvironment.keyPair().publicKey()) ==
        cosigners.end()) {
        m_executorEnvironment.storage().synchronizeStorage(m_contractEnvironment.driveKey(), info.m_driveState);
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

    m_executorEnvironment.storage().synchronizeStorage(m_contractEnvironment.driveKey(), m_request.m_storageHash);
}

void SynchronizationTask::terminate() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_contractEnvironment.finishTask();
}

}
