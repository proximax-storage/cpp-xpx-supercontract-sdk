/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "virtualMachine/VirtualMachine.h"
#include "supercontract/StorageObserver.h"
#include "supercontract/AsyncQuery.h"

#include "supercontract/SingleThread.h"
#include "virtualMachine/VirtualMachineInternetQueryHandler.h"
#include "virtualMachine/VirtualMachineBlockchainQueryHandler.h"
#include "ExecuteCallRPCRequest.h"

namespace sirius::contract::vm {

class RPCVirtualMachine
        : public VirtualMachine, private SingleThread {

private:

    struct CallContext {
        ExecuteCallRPCRequest m_request;
        std::shared_ptr<AsyncQuery> m_query;

        CallContext(ExecuteCallRPCRequest&& request, std::shared_ptr<AsyncQuery>);

        CallContext(CallContext&&) = default;

        CallContext& operator=(CallContext&& other) noexcept = default;
    };

    const StorageObserver& m_storageObserver;

    GlobalEnvironment& m_environment;

    std::unique_ptr<supercontractserver::SupercontractServer::Stub> m_stub;

    grpc::CompletionQueue m_completionQueue;
    std::thread m_completionQueueThread;

    std::map<CallId, std::shared_ptr<AsyncQuery>> m_pathQueries;

    std::map<CallId, CallContext> m_callContexts;

public:

    RPCVirtualMachine(
            const StorageObserver& storageObserver,
            GlobalEnvironment& environment,
            const std::string& serverAddress);

    ~RPCVirtualMachine() override;

    void executeCall(const ContractKey& contractKey,
                     const CallRequest& request,
                     std::weak_ptr<VirtualMachineInternetQueryHandler>&& internetQueryHandler,
                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>&& blockchainQueryHandler,
                     std::shared_ptr<AsyncQueryCallback<std::optional<CallExecutionResult>>> callback) override;

private:

    void
    onReceivedCallAbsolutePath(CallRequest&& request,
                               std::string&& callAbsolutePath,
                               std::weak_ptr<VirtualMachineInternetQueryHandler>&& internetQueryHandler,
                               std::weak_ptr<VirtualMachineBlockchainQueryHandler>&& blockchainQueryHandler,
                               std::shared_ptr<AsyncQueryCallback<std::optional<CallExecutionResult>>>&& callback);

    void waitForRPCResponse();

    void onCallExecuted(const CallId&);
};

}