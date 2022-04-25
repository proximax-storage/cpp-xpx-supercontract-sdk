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

    std::optional<CallExecutionResult> m_callExecutionResult;

public:

    CallExecutionEnvironment( const CallRequest& request ) : m_callRequest( request ) {}

    const CallId& callId() const {
        return m_callRequest.m_callId;
    }

    const CallRequest& callRequest() const {
        return m_callRequest;
    }

    const CallExecutionResult& callExecutionResult() const {
        return *m_callExecutionResult;
    }

    void setCallExecutionResult( const CallExecutionResult& callExecutionResult ) {
        m_callExecutionResult = callExecutionResult;
    }

};

}