/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <virtualMachine/VirtualMachineErrorCode.h>
#include "ExecuteCallRPCStarter.h"

namespace sirius::contract::vm {

ExecuteCallRPCStarter::ExecuteCallRPCStarter(
        GlobalEnvironment& environment,
        std::shared_ptr<AsyncQueryCallback<void>> callback)
        : m_environment(environment)
        , m_callback(std::move(callback)) {}

void ExecuteCallRPCStarter::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger());

    if (!ok) {
        m_callback->postReply(tl::unexpected<std::error_code>(make_error_code(VirtualMachineError::vm_unavailable)));    }
    else {
        m_callback->postReply(expected<void>());
    }
}

}