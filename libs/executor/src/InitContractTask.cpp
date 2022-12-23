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

    m_contractEnvironment.finishTask();
}


void InitContractTask::terminate() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(false, m_executorEnvironment.logger())
}

// region blockchain event handler

bool InitContractTask::onEndBatchExecutionPublished(const PublishedEndBatchExecutionTransactionInfo& info) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(false, m_executorEnvironment.logger())

    return true;
}

// endregion

}