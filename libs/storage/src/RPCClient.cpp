/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <grpcpp/security/credentials.h>
#include <grpcpp/create_channel.h>
#include "RPCClient.h"


namespace sirius::contract::storage {

RPCClient::RPCClient(GlobalEnvironment& environment, const std::string& serverAddress)
        : m_environment(environment), m_stub(storageServer::StorageServer::NewStub(
        grpc::CreateChannel(serverAddress, grpc::InsecureChannelCredentials()))), m_completionQueueThread([this] {
    waitForRPCResponse();
}) {}

RPCClient::~RPCClient() {
    ASSERT(isSingleThread(), m_environment.logger())

    for (auto* pTag : m_activeTags) {
        pTag->cancel();
    }

    m_completionQueue.Shutdown();

    if (m_completionQueueThread.joinable()) {
        m_completionQueueThread.join();
    }
}

void RPCClient::waitForRPCResponse() {
    ASSERT(!isSingleThread(), m_environment.logger())
    void* pTag;
    bool ok;
    while (m_completionQueue.Next(&pTag, &ok)) {
        auto* pQuery = static_cast<RPCTag*>(pTag);
        ASSERT(m_activeTags.contains(pQuery), m_environment.logger())
        m_activeTags.erase(pQuery);
        pQuery->process(ok);
        delete pQuery;
    }
}

storageServer::StorageServer::Stub& RPCClient::stub() {
    return *m_stub;
}

grpc::CompletionQueue& RPCClient::completionQueue() {
    return m_completionQueue;
}

void RPCClient::addTag(RPCTag* tag) {
    m_activeTags.insert(tag);
}

}