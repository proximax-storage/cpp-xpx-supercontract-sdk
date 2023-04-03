/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "WriteRPCTag.h"

namespace sirius::contract::rpcExecutorServer {

WriteRPCTag::WriteRPCTag(std::promise<bool>&& promise)
: m_promise(std::move(promise)) {}

void WriteRPCTag::process(bool ok) {
    m_promise.set_value(ok);
}

}