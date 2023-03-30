/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "WriteRPCTag.h"
#include "RPCExecutorClientErrorCode.h"

namespace sirius::contract::executor {

WriteRPCTag::WriteRPCTag(RPCTagListener& listener)
        : m_listener(listener) {}

void WriteRPCTag::process(bool ok) {
    if (!ok) {
        m_listener.onWritten(tl::make_unexpected<std::error_code>(
                make_error_code(RPCExecutorError::write_error)));
        return;
    }

    m_listener.onWritten(tl::expected<void, std::error_code>());
}

}
