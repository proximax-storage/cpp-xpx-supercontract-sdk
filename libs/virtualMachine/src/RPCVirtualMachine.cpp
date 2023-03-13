/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "RPCVirtualMachine.h"
#include "ExecuteCallRPCRequest.h"
#include "RPCTag.h"
#include <virtualMachine/VirtualMachineErrorCode.h>
#include "supercontract_server.grpc.pb.h"
#include <grpcpp/create_channel.h>

namespace sirius::contract::vm {

RPCVirtualMachine::RPCVirtualMachine(std::weak_ptr<storage::Storage> storage,
                                     GlobalEnvironment& environment,
                                     const std::string& serverAddress)
        : m_storage(std::move(storage)), m_environment(environment), m_stub(
        supercontractserver::SupercontractServer::NewStub(grpc::CreateChannel(
                serverAddress, grpc::InsecureChannelCredentials()))), m_completionQueueThread([this] {
    waitForRPCResponse();
}) {}

RPCVirtualMachine::~RPCVirtualMachine() {

    ASSERT(isSingleThread(), m_environment.logger())

    m_pathQueries.clear();

    m_callContexts.clear();

    m_completionQueue.Shutdown();
    if (m_completionQueueThread.joinable()) {
        m_completionQueueThread.join();
    }
}

void RPCVirtualMachine::executeCall(const CallRequest& request,
                                    std::weak_ptr<VirtualMachineInternetQueryHandler> internetQueryHandler,
                                    std::weak_ptr<VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                                    std::weak_ptr<VirtualMachineStorageQueryHandler> storageQueryHandler,
                                    std::shared_ptr<AsyncQueryCallback<CallExecutionResult>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    m_environment.logger().info("Call execution is started by RPC VM. "
                                "CallId: {}, call level: {}",
                                request.m_callId,
                                static_cast<uint8_t>(request.m_callLevel));

    auto storage = m_storage.lock();

    if (!storage) {
        callback->postReply(
                tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[pathQuery, pathCallback] = createAsyncQuery<std::string>(
            [=, this, request = request, internetQueryHandler = std::move(
                    internetQueryHandler), blockchainQueryHandler = std::move(
                    blockchainQueryHandler), storageQueryHandler = std::move(storageQueryHandler)](
                    auto&& callAbsolutePath) mutable -> void {
                onReceivedCallAbsolutePath(std::move(request),
                                           std::forward<decltype(callAbsolutePath)>(callAbsolutePath),
                                           std::move(internetQueryHandler),
                                           std::move(blockchainQueryHandler),
                                           std::move(storageQueryHandler),
                                           std::move(callback));
            },
            [=] {
                callback->postReply(
                        tl::unexpected<std::error_code>(make_error_code(VirtualMachineError::vm_unavailable)));
            },
            m_environment, true, true);
    m_pathQueries[request.m_callId] = std::move(pathQuery);
    storage->absolutePath(DriveKey(), request.m_file, pathCallback);
}

void RPCVirtualMachine::onReceivedCallAbsolutePath(CallRequest&& request,
                                                   expected<std::string>&& callAbsolutePath,
                                                   std::weak_ptr<VirtualMachineInternetQueryHandler>&& internetQueryHandler,
                                                   std::weak_ptr<VirtualMachineBlockchainQueryHandler>&& blockchainQueryHandler,
                                                   std::weak_ptr<VirtualMachineStorageQueryHandler> storageQueryHandler,
                                                   std::shared_ptr<AsyncQueryCallback<CallExecutionResult>>&& callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    auto callId = request.m_callId;

    ASSERT(!m_callContexts.contains(callId), m_environment.logger())

    m_pathQueries.erase(request.m_callId);

    if (!callAbsolutePath) {
        if (callAbsolutePath.error() == std::errc::no_such_file_or_directory) {
            CallExecutionResult executionResult;
            executionResult.m_success = false;
            executionResult.m_return = 0;
            executionResult.m_execution_gas_consumed = 0;
            executionResult.m_download_gas_consumed = 0;
            executionResult.m_proofOfExecutionSecretData = request.m_proofOfExecutionPrefix;
            callback->postReply(executionResult);
        } else {
            callback->postReply(tl::unexpected(callAbsolutePath.error()));
        }
        return;
    }

    request.m_file = *callAbsolutePath;

    auto[vmQuery, vmCallback] = createAsyncQuery<CallExecutionResult>(
            [=, this](expected<CallExecutionResult>&& result) {
                callback->postReply(std::move(result));
                onCallExecuted(callId);
            },
            [=] {
                callback->postReply(
                        tl::unexpected<std::error_code>(make_error_code(VirtualMachineError::vm_unavailable)));
            }, m_environment, true, false);

    auto call = ExecuteCallRPCRequest(m_environment,
                                      std::move(request),
                                      *m_stub,
                                      m_completionQueue,
                                      std::move(internetQueryHandler),
                                      std::move(blockchainQueryHandler),
                                      std::move(storageQueryHandler),
                                      std::move(vmCallback));

    m_callContexts.emplace(callId, CallContext(std::move(call), vmQuery));
}

void RPCVirtualMachine::waitForRPCResponse() {

    ASSERT(!isSingleThread(), m_environment.logger())

    void* pTag;
    bool ok;
    while (m_completionQueue.Next(&pTag, &ok)) {
        auto* pQuery = static_cast<RPCTag*>(pTag);
        pQuery->process(ok);
        delete pQuery;
    }
}

void RPCVirtualMachine::onCallExecuted(const CallId& callId) {

    ASSERT(isSingleThread(), m_environment.logger())

    auto it = m_callContexts.find(callId);

    ASSERT(it != m_callContexts.end(), m_environment.logger())

    m_callContexts.erase(it);
}

RPCVirtualMachine::CallContext::CallContext(ExecuteCallRPCRequest&& request, std::shared_ptr<AsyncQuery> query)
        : m_request(std::move(request)), m_query(std::move(query)) {}

} // namespace sirius::contract::vm