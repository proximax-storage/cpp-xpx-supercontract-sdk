/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ExecuteCallRPCRequest.h"

namespace sirius::contract::vm {

ExecuteCallRPCRequest::ExecuteCallRPCRequest(GlobalEnvironment& environment, CallRequest&& callRequest,
                                             supercontractserver::SupercontractServer::Stub& stub,
                                             grpc::CompletionQueue& completionQueue,
                                             std::weak_ptr<VirtualMachineInternetQueryHandler>&& internetQueryHandler,
                                             std::weak_ptr<VirtualMachineBlockchainQueryHandler>&& blockchainQueryHandler,
                                             std::weak_ptr<VirtualMachineStorageQueryHandler>&& storageQueryHandler,
                                             std::shared_ptr<AsyncQueryCallback<CallExecutionResult>>&& callback)
    : m_environment(environment), m_handler(std::make_shared<ExecuteCallRPCHandler>(
                                      environment, std::move(callRequest), stub, completionQueue, std::move(internetQueryHandler),
                                      std::move(blockchainQueryHandler), std::move(storageQueryHandler), std::move(callback))) {
    m_handler->start();
}

ExecuteCallRPCRequest::ExecuteCallRPCRequest(ExecuteCallRPCRequest&& other) noexcept
    : m_environment(other.m_environment), m_handler(std::move(other.m_handler)) {}

ExecuteCallRPCRequest& ExecuteCallRPCRequest::operator=(ExecuteCallRPCRequest&& other) noexcept {
    m_environment = other.m_environment;
    m_handler = std::move(other.m_handler);
    return *this;
}

ExecuteCallRPCRequest::~ExecuteCallRPCRequest() {

    ASSERT(isSingleThread(), m_environment.logger())

    if (m_handler) {
        m_handler->finish();
    }
}

} // namespace sirius::contract::vm