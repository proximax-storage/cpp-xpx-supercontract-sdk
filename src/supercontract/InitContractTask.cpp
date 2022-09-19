/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "InitContractTask.h"

namespace sirius::contract {

InitContractTask::InitContractTask(
        AddContractRequest&& request,
        ContractEnvironment& contractEnvironment,
        ExecutorEnvironment& executorEnvironment)
        : BaseContractTask(executorEnvironment, contractEnvironment)
        , m_request(std::move(request)) {}

void InitContractTask::run() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_request.m_batchesExecuted > 0) {
        // The corresponding storage is not yet fully controlled by the Contract,
        // so the Storage performs necessary synchronization by himself
        m_contractEnvironment.finishTask();
    }
}


void InitContractTask::terminate() {
    m_contractEnvironment.finishTask();
}

// region blockchain event handler

bool InitContractTask::onEndBatchExecutionPublished(const PublishedEndBatchExecutionTransactionInfo& info) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    const auto& cosigners = info.m_cosigners;

    if (std::find(cosigners.begin(), cosigners.end(), m_executorEnvironment.keyPair().publicKey()) !=
        cosigners.end()) {
        // We are in the actual state from the point of blockchain view
        // At the same time storage actually may not be in the actual state
        // So we should try to synchronize storage without expecting approval in the blockchain
        m_executorEnvironment.storage().synchronizeStorage(m_contractEnvironment.driveKey(), info.m_driveState);
    } else {
        m_contractEnvironment.addSynchronizationTask(SynchronizationRequest{info.m_driveState});
    }

    m_contractEnvironment.finishTask();

    return true;
}

// endregion

}