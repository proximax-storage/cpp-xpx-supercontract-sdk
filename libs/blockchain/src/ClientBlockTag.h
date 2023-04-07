/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "ClientRPCTag.h"
#include <common/SingleThread.h>
#include <common/GlobalEnvironment.h>
#include <common/AsyncQuery.h>
#include "blockchain.grpc.pb.h"
#include <blockchain/Block.h>

namespace sirius::contract::blockchain {

class ClientBlockTag
        : private SingleThread, public ClientRPCTag {

private:

    GlobalEnvironment& m_environment;

    blockchainServer::BlockRequest m_request;
    blockchainServer::BlockResponse m_response;

    grpc::ClientContext m_context;
    std::unique_ptr<
            grpc::ClientAsyncResponseReader<
                    blockchainServer::BlockResponse>> m_responseReader;

    grpc::Status m_status;

    std::shared_ptr<AsyncQueryCallback<Block>> m_callback;

public:

    ClientBlockTag(GlobalEnvironment& environment,
                   blockchainServer::BlockRequest&& request,
                   blockchainServer::BlockchainServer::Stub& stub,
                   grpc::CompletionQueue& completionQueue,
                   std::shared_ptr<AsyncQueryCallback<Block>>&& callback);

    void start();

    void process(bool ok) override;

    void cancel() override;
};

}
