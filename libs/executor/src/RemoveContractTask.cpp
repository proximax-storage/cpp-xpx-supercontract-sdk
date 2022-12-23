/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "RemoveContractTask.h"

namespace sirius::contract {

RemoveContractTask::RemoveContractTask(
        RemoveRequest&& request,
        ContractEnvironment& contractEnvironment,
        ExecutorEnvironment& executorEnvironment)
        : BaseContractTask(executorEnvironment, contractEnvironment)
        , m_request(std::move(request)) {}

void RemoveContractTask::run() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_contractEnvironment.finishTask();
}

void RemoveContractTask::terminate() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_contractEnvironment.finishTask();
}

};