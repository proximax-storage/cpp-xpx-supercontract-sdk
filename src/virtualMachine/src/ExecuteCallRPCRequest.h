/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "ExecuteCallRPCHandler.h"
#include "virtualMachine/CallExecutionResult.h"

namespace sirius::contract::vm {

class ExecuteCallRPCRequest: private SingleThread{

private:

    GlobalEnvironment& m_environment;
    std::shared_ptr<ExecuteCallRPCHandler> m_handler;

public:

    ExecuteCallRPCRequest(GlobalEnvironment& environment,
                          CallRequest&& callRequest,
                          supercontractserver::SupercontractServer::Stub& stub,
                          grpc::CompletionQueue& completionQueue,
                          std::weak_ptr<VirtualMachineInternetQueryHandler>&& internetQueryHandler,
                          std::weak_ptr<VirtualMachineBlockchainQueryHandler>&& blockchainQueryHandler,
                          std::shared_ptr<AsyncQueryCallback<std::optional<CallExecutionResult>>>&& callback);

    ExecuteCallRPCRequest(const ExecuteCallRPCRequest&) = delete;
    ExecuteCallRPCRequest(ExecuteCallRPCRequest&&) noexcept;
    ExecuteCallRPCRequest& operator=(const ExecuteCallRPCRequest&) = delete;
    ExecuteCallRPCRequest& operator=(ExecuteCallRPCRequest&& other) noexcept;

    ~ExecuteCallRPCRequest();
};

}

