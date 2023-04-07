/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <grpcpp/create_channel.h>
#include "RPCBlockchainClient.h"
#include "blockchain.grpc.pb.h"
#include "ClientBlockTag.h"

namespace sirius::contract::blockchain {

RPCBlockchainClient::RPCBlockchainClient(GlobalEnvironment& environment, const std::string& serverAddress)
        : m_environment(environment)
          , m_stub(blockchainServer::BlockchainServer::NewStub(
                grpc::CreateChannel(serverAddress, grpc::InsecureChannelCredentials())))
          , m_completionQueueThread([this] {
            waitForRPCResponse();
        }) {}

RPCBlockchainClient::~RPCBlockchainClient() {
    ASSERT(isSingleThread(), m_environment.logger())

    {
        std::lock_guard<std::mutex> guard(m_activeTagsMutex);
        for (auto* pTag : m_activeTags) {
            pTag->cancel();
        }
    }

    m_completionQueue.Shutdown();

    if (m_completionQueueThread.joinable()) {
        m_completionQueueThread.join();
    }
}

void RPCBlockchainClient::waitForRPCResponse() {
    ASSERT(!isSingleThread(), m_environment.logger())
    void* pTag;
    bool ok;
    while (m_completionQueue.Next(&pTag, &ok)) {
        auto* pQuery = static_cast<ClientRPCTag*>(pTag);
        {
            std::lock_guard<std::mutex> guard(m_activeTagsMutex);
            ASSERT(m_activeTags.contains(pQuery), m_environment.logger())
            m_activeTags.erase(pQuery);
        }
        pQuery->process(ok);
        delete pQuery;
    }
}

void RPCBlockchainClient::block(uint64_t height, std::shared_ptr<AsyncQueryCallback<Block>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    blockchainServer::BlockRequest request;
    request.set_height(height);

    auto* tag = new ClientBlockTag(m_environment, std::move(request), *m_stub, m_completionQueue, std::move(callback));
    tag->start();
}

}