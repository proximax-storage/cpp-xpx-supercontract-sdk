/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <grpcpp/impl/codegen/server_context.h>
#include "ServerRPCTag.h"
#include "blockchain.grpc.pb.h"

namespace sirius::contract::blockchain {

class ServerBlockTag: public ServerRPCTag {

    grpc::ServerContext m_serverContext;
    grpc::ServerAsyncResponseWriter<blockchainServer::BlockResponse> m_responder;


};

}