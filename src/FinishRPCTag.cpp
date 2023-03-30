/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "FinishRPCTag.h"
#include "RPCExecutorClientErrorCode.h"

namespace sirius::contract::executor {

FinishRPCTag::FinishRPCTag(RPCTagListener& listener)
        : m_listener(listener) {}

void FinishRPCTag::process(bool ok) {
    if (!ok) {
        m_listener.onFinished(tl::make_unexpected<std::error_code>(
                make_error_code(RPCExecutorError::finish_error)));
        return;
    }

    m_listener.onFinished(tl::expected<void, std::error_code>());
}

}