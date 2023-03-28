/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ExecuteCallRPCFinisher.h"

namespace sirius::contract::vm {

ExecuteCallRPCFinisher::ExecuteCallRPCFinisher(
        GlobalEnvironment& environment,
        std::shared_ptr<AsyncQueryCallback<grpc::Status>> callback)
        : m_environment(environment)
        , m_callback(std::move(callback)) {}

void ExecuteCallRPCFinisher::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger());

    ASSERT(ok, m_environment.logger())

    m_callback->postReply(std::move(m_status));
}

}