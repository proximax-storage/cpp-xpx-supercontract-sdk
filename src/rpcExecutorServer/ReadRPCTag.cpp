/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ReadRPCTag.h"

namespace sirius::contract::rpcExecutorServer {

ReadRPCTag::ReadRPCTag(std::promise<tl::expected<executor_server::ClientMessage, std::error_code>>&& promise)
: m_promise(std::move(promise)) {}

void ReadRPCTag::process(bool ok) {
    if (!ok) {
        return;
    }
    m_promise.set_value(std::move(m_clientMessage));
}

}