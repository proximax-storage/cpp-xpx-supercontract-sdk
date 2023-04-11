/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include "blockchain.pb.h"

namespace sirius::contract::blockchain {

class BlockContext {

public:

    grpc::ServerContext m_context;
    blockchainServer::BlockRequest m_request;
    grpc::ServerAsyncResponseWriter<blockchainServer::BlockResponse> m_responder;

public:

    BlockContext();

};

}