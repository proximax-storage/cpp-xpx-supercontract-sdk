/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCTag.h"
#include "RPCTagListener.h"

namespace sirius::contract::executor {

class ReadRPCTag: public RPCTag {

private:

    RPCTagListener& m_listener;

public:

    executor_server::RunExecutorResponse m_response;

public:

    ReadRPCTag(RPCTagListener& listener);

    void process(bool ok) override;

};

}