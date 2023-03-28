/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCTag.h"
#include <grpcpp/support/status.h>
#include <common/SingleThread.h>
#include <common/AsyncQuery.h>

namespace sirius::contract::messenger {

class FinishSessionTag: public RPCTag, private SingleThread {

private:

    GlobalEnvironment& m_environment;

public:

    grpc::Status m_status;

public:

    FinishSessionTag(GlobalEnvironment& environment);

    void process(bool ok) override;

};

}