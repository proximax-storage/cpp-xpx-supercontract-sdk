/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "types.h"
#include "eventHandlers/ContractVirtualMachineQueryHandler.h"

namespace sirius::contract {

class CallExecutionEnvironment {

private:
    const CallRequest m_callRequest;

public:

    CallExecutionEnvironment( const CallRequest& request ) : m_callRequest( request ) {}

    const Hash256& callId() {
        return m_callRequest.m_callId;
    }

    const CallRequest& callRequest() {
        return m_callRequest;
    }

};

}