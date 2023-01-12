/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "AutomaticExecutionBlockchainQueryHandler.h"

namespace sirius::contract {

AutomaticExecutionBlockchainQueryHandler::AutomaticExecutionBlockchainQueryHandler(
        ExecutorEnvironment& executorEnvironment,
        ContractEnvironment& contractEnvironment,
        const CallerKey& callerKey, uint64_t blockHeight)
        : AutorunBlockchainQueryHandler(executorEnvironment, contractEnvironment, blockHeight)
        , m_caller(callerKey) {}

void AutomaticExecutionBlockchainQueryHandler::callerPublicKey(
        std::shared_ptr<AsyncQueryCallback<CallerKey>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    callback->postReply(m_caller);
}

}
