/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ReadRPCTag.h"
#include "RPCExecutorClientErrorCode.h"

namespace sirius::contract::executor {

ReadRPCTag::ReadRPCTag(RPCTagListener& listener)
        : m_listener(listener) {}

void ReadRPCTag::process(bool ok) {
    if (!ok) {
        m_listener.onRead(tl::make_unexpected<std::error_code>(
                make_error_code(RPCExecutorError::read_error)));
        return;
    }

    m_listener.onRead(m_response);
}

}
