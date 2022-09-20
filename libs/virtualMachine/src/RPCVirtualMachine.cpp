/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "RPCVirtualMachine.h"

#include "RPCTag.h"
#include "ExecuteCallRPCRequest.h"

#include "supercontract_server.grpc.pb.h"
#include <grpcpp/create_channel.h>

namespace sirius::contract::vm {

RPCVirtualMachine::RPCVirtualMachine(std::weak_ptr<storage::StorageObserver> storageObserver,
                                     GlobalEnvironment& environment,
                                     const std::string& serverAddress)
        : m_storageObserver(std::move(storageObserver))
        , m_environment(environment)
        , m_stub(std::move(supercontractserver::SupercontractServer::NewStub(grpc::CreateChannel(
                serverAddress, grpc::InsecureChannelCredentials()))))
        , m_completionQueueThread([this] {
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
                                    std::shared_ptr<AsyncQueryCallback<std::optional<CallExecutionResult>>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    auto storageObserver = m_storageObserver.lock();

    if (!storageObserver) {
        callback->postReply({});
        return;
    }

    auto c = [=,
            this,
            request = request,
            internetQueryHandler = std::move(internetQueryHandler),
            blockchainQueryHandler = std::move(blockchainQueryHandler)](
            std::string&& callAbsolutePath) mutable -> void {
        onReceivedCallAbsolutePath(std::move(request),
                                   std::move(callAbsolutePath),
                                   std::move(internetQueryHandler),
                                   std::move(blockchainQueryHandler),
                                   std::move(callback));
    };
    auto [pathQuery, pathCallback] = createAsyncQuery<std::string>(std::move(c), [=] {
        callback->postReply({});
    }, m_environment, true, true);
    m_pathQueries[request.m_callId] = std::move(pathQuery);
    storageObserver->getAbsolutePath(request.m_file, pathCallback);
}

void RPCVirtualMachine::onReceivedCallAbsolutePath(CallRequest&& request,
                                                   std::string&& callAbsolutePath,
                                                   std::weak_ptr<VirtualMachineInternetQueryHandler>&& internetQueryHandler,
                                                   std::weak_ptr<VirtualMachineBlockchainQueryHandler>&& blockchainQueryHandler,
                                                   std::shared_ptr<AsyncQueryCallback<std::optional<CallExecutionResult>>>&& callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    auto callId = request.m_callId;

    ASSERT(!m_callContexts.contains(callId), m_environment.logger());

    m_pathQueries.erase(request.m_callId);

    request.m_file = callAbsolutePath;

    auto [vmQuery, vmCallback] = createAsyncQuery<std::optional<CallExecutionResult>>(
            [=, this](std::optional<CallExecutionResult>&& result) {
                callback->postReply(std::move(result));
                onCallExecuted(callId);
            }, [=] { callback->postReply({}); }, m_environment, true, false);

    auto call = ExecuteCallRPCRequest(m_environment,
                                      std::move(request),
                                      *m_stub,
                                      m_completionQueue,
                                      std::move(internetQueryHandler),
                                      std::move(blockchainQueryHandler),
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
        : m_request(std::move(request))
        , m_query(std::move(query)) {}

}