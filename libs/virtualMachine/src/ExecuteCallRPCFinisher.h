/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <grpcpp/support/status.h>
#include <common/AsyncQuery.h>
#include <common/SingleThread.h>
#include "RPCTag.h"

#include "supercontract_server.pb.h"

namespace sirius::contract::vm {

class ExecuteCallRPCFinisher:
        private SingleThread,
        public RPCTag {

private:

    GlobalEnvironment& m_environment;

    std::shared_ptr<AsyncQueryCallback<grpc::Status>> m_callback;

public:

    grpc::Status m_status;

public:

    explicit ExecuteCallRPCFinisher(
            GlobalEnvironment&,
            std::shared_ptr<AsyncQueryCallback<grpc::Status>> );

    void process( bool ok ) override;
};

}