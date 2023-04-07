/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <common/SingleThread.h>
#include <common/GlobalEnvironment.h>
#include "blockchain.grpc.pb.h"
#include "ClientRPCTag.h"
#include <blockchain/Blockchain.h>

namespace sirius::contract::blockchain {

class RPCBlockchainClient: private SingleThread, public Blockchain {

private:
    GlobalEnvironment& m_environment;

    std::unique_ptr<blockchainServer::BlockchainServer::Stub> m_stub;

    grpc::CompletionQueue m_completionQueue;
    std::thread m_completionQueueThread;

    // This set does NOT own the tags
    std::set<ClientRPCTag*> m_activeTags;
    std::mutex m_activeTagsMutex;

public:

    RPCBlockchainClient(
            GlobalEnvironment& environment,
            const std::string& serverAddress);

    ~RPCBlockchainClient();

public:

    void block(uint64_t height, std::shared_ptr<AsyncQueryCallback<Block>> callback) override;

private:
    void waitForRPCResponse();

};

}