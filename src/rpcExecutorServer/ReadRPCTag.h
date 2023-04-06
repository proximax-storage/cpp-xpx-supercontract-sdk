/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "RPCTag.h"
#include <future>
#include "executor.pb.h"
#include <tl/expected.hpp>

namespace sirius::contract::rpcExecutorServer {

class ReadRPCTag: public RPCTag {

private:

    std::promise<tl::expected<executor_server::ClientMessage, std::error_code>> m_promise;

public:

    executor_server::ClientMessage m_clientMessage;

public:

    ReadRPCTag(std::promise<tl::expected<executor_server::ClientMessage, std::error_code>>&& promise);

    void process(bool ok) override;
};

}