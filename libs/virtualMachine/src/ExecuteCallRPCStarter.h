/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/AsyncQuery.h"
#include "supercontract/SingleThread.h"
#include "RPCTag.h"

#include "supercontract_server.pb.h"

namespace sirius::contract::vm {

class ExecuteCallRPCStarter
        :
                private SingleThread,
                public RPCTag {

private:
    std::shared_ptr<AsyncQueryCallback<void>> m_callback;

    GlobalEnvironment& m_environment;

public:

    explicit ExecuteCallRPCStarter(
            GlobalEnvironment& environment,
            std::shared_ptr<AsyncQueryCallback<void>> callback);

    void process(bool ok) override;
};

}