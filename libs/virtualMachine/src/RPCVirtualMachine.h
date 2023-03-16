/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "ExecuteCallRPCRequest.h"
#include "supercontract/AsyncQuery.h"
#include "supercontract/SingleThread.h"
#include <storage/Storage.h>
#include <virtualMachine/VirtualMachine.h>
#include <virtualMachine/VirtualMachineBlockchainQueryHandler.h>
#include <virtualMachine/VirtualMachineInternetQueryHandler.h>
#include <virtualMachine/VirtualMachineStorageQueryHandler.h>

namespace sirius::contract::vm {

class RPCVirtualMachine
    : public VirtualMachine,
      private SingleThread {

private:
    struct CallContext {
        ExecuteCallRPCRequest m_request;
        std::shared_ptr<AsyncQuery> m_query;

        CallContext(ExecuteCallRPCRequest&& request, std::shared_ptr<AsyncQuery>);

        CallContext(CallContext&&) = default;

        CallContext& operator=(CallContext&& other) noexcept = default;
    };

    std::weak_ptr<storage::Storage> m_storage;

    GlobalEnvironment& m_environment;

    std::unique_ptr<supercontractserver::SupercontractServer::Stub> m_stub;

    grpc::CompletionQueue m_completionQueue;
    std::thread m_completionQueueThread;

    std::map<CallId, std::shared_ptr<AsyncQuery>> m_fileInfoQueries;

    std::map<CallId, CallContext> m_callContexts;

public:
    RPCVirtualMachine(
        std::weak_ptr<storage::Storage> storage,
        GlobalEnvironment& environment,
        const std::string& serverAddress);

    ~RPCVirtualMachine() override;

    void executeCall(const CallRequest& request,
                     std::weak_ptr<VirtualMachineInternetQueryHandler> internetQueryHandler,
                     std::weak_ptr<VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                     std::weak_ptr<VirtualMachineStorageQueryHandler> storageQueryHandler,
                     std::shared_ptr<AsyncQueryCallback<CallExecutionResult>> callback) override;

private:
    void
    onReceivedCallFileInfo(CallRequest&& request,
                           expected<storage::FileInfo>&& fileInfo,
                           std::weak_ptr<VirtualMachineInternetQueryHandler>&& internetQueryHandler,
                           std::weak_ptr<VirtualMachineBlockchainQueryHandler>&& blockchainQueryHandler,
                           std::weak_ptr<VirtualMachineStorageQueryHandler> storageQueryHandler,
                           std::shared_ptr<AsyncQueryCallback<CallExecutionResult>>&& callback);

    void waitForRPCResponse();

    void onCallExecuted(const CallId&);
};

} // namespace sirius::contract::vm