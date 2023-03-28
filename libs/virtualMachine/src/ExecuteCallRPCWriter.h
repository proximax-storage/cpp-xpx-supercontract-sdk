/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <common/AsyncQuery.h>
#include <common/SingleThread.h>
#include "RPCTag.h"

namespace sirius::contract::vm {

class ExecuteCallRPCWriter
        :
                private SingleThread,
                public RPCTag {

    GlobalEnvironment& m_environment;

    std::shared_ptr<AsyncQueryCallback<void>> m_callback;

public:

    explicit ExecuteCallRPCWriter(
            GlobalEnvironment& environment,
            std::shared_ptr<AsyncQueryCallback<void>> callback);

    void process(bool ok) override;
};

}