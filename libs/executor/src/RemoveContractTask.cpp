/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "RemoveContractTask.h"

namespace sirius::contract {

RemoveContractTask::RemoveContractTask(
        RemoveRequest&& request,
        std::shared_ptr<AsyncQueryCallback<void>>&& onTaskFinishedCallback,
        ContractEnvironment& contractEnvironment,
        ExecutorEnvironment& executorEnvironment)
        : BaseContractTask(executorEnvironment, contractEnvironment)
        , m_request(std::move(request))
        , m_onTaskFinishedCallback(std::move(onTaskFinishedCallback)) {}

void RemoveContractTask::run() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Remove contract task is run. "
                                        "Contract key: {} ",
                                        m_contractEnvironment.contractKey());

    m_onTaskFinishedCallback->postReply(expected<void>());
}

void RemoveContractTask::terminate() {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.logger().warn("Remove execution task is terminated. "
                                        "Contract key: {}",
                                        m_contractEnvironment.contractKey());
}

}