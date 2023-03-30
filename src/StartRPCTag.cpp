/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "StartRPCTag.h"
#include "RPCExecutorClientErrorCode.h"

namespace sirius::contract::executor {

StartRPCTag::StartRPCTag(RPCTagListener& listener)
        : m_listener(listener) {}

void StartRPCTag::process(bool ok) {
    if (!ok) {
        m_listener.onStarted(tl::make_unexpected<std::error_code>(
                make_error_code(RPCExecutorError::start_error)));
        return;
    }

    m_listener.onStarted(tl::expected<void, std::error_code>());
}

}