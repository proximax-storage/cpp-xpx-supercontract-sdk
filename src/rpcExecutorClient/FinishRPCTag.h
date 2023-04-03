/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCTag.h"
#include "RPCTagListener.h"

#include <grpcpp/support/status.h>

namespace sirius::contract::rpcExecutorClient {

class FinishRPCTag: public RPCTag {

private:

    RPCTagListener& m_listener;

public:

    grpc::Status m_status;

public:

    FinishRPCTag(RPCTagListener& listener);

    void process(bool ok) override;

};

}