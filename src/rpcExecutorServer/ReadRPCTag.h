/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "RPCTag.h"
#include "executor.pb.h"
#include <tl/expected.hpp>
#include <common/AsyncQuery.h>

namespace sirius::contract::rpcExecutorServer {

class ReadRPCTag: public RPCTag {

private:

    std::shared_ptr<AsyncQueryCallback<executor_server::ClientMessage>> m_callback;

public:

    executor_server::ClientMessage m_clientMessage;

public:

    ReadRPCTag(std::shared_ptr<AsyncQueryCallback<executor_server::ClientMessage>>&& callback);

    void process(bool ok) override;
};

}