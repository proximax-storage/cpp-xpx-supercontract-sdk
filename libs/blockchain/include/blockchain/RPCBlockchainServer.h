/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "blockchain.grpc.pb.h"
#include <blockchain/Blockchain.h>
#include <common/AsyncQuery.h>
#include "BlockContext.h"
#include <thread>
#include <memory>
#include <grpcpp/server_builder.h>

namespace sirius::contract::blockchain {

class RPCBlockchainServer: private SingleThread {

private:

    GlobalEnvironment& m_environment;

    blockchainServer::BlockchainServer::AsyncService m_service;

    std::unique_ptr<grpc::ServerCompletionQueue> m_completionQueue;
    std::thread m_completionQueueThread;

    std::unique_ptr<Blockchain> m_blockchain;

    std::shared_ptr<AsyncQuery> m_blockServerRequest;

    std::map<uint64_t, std::shared_ptr<AsyncQuery>> m_blockchainQueries;
    uint64_t m_blockchainQueriesCounter = 0;

public:

    RPCBlockchainServer(GlobalEnvironment& environment,
                        grpc::ServerBuilder& builder,
                        std::unique_ptr<blockchain::Blockchain>&& blockchain);

	void start();

    ~RPCBlockchainServer();

private:

    void acceptBlockRequest();

    void onBlockRequestReceived(std::shared_ptr<BlockContext>&& context);

    void onBlockReceived(uint64_t queryId, std::shared_ptr<BlockContext>&& context, const blockchain::Block& block);

    void waitForRPCResponse();
};

}