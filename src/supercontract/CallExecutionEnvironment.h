/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "types.h"
#include "contract/AsyncQuery.h"

namespace sirius::contract {

class CallExecutionEnvironment: public VirtualMachineInternetQueryHandler {

private:

    ExecutorEnvironment& m_executorEnvironment;
    ContractEnvironment& m_contractEnvironment;

    const CallRequest m_callRequest;

    std::optional<CallExecutionResult> m_callExecutionResult;

    std::shared_ptr<AsyncQuery> m_asyncQuery;

    const DebugInfo m_dbgInfo;

public:

    CallExecutionEnvironment( const CallRequest& request,
                              ExecutorEnvironment& executorEnvironment,
                              ContractEnvironment& contractEnvironment,
                              const DebugInfo& debugInfo )
    : m_callRequest( request )
    , m_executorEnvironment( executorEnvironment )
    , m_contractEnvironment( contractEnvironment )
    , m_dbgInfo( debugInfo )
    {}

    const CallId& callId() const {

        DBG_MAIN_THREAD

        return m_callRequest.m_callId;
    }

    const CallRequest& callRequest() const {

        DBG_MAIN_THREAD

        return m_callRequest;
    }

    const CallExecutionResult& callExecutionResult() const {

        DBG_MAIN_THREAD

        return *m_callExecutionResult;
    }

    void setCallExecutionResult( const CallExecutionResult& callExecutionResult ) {

        DBG_MAIN_THREAD

        m_callExecutionResult = callExecutionResult;
    }

    void terminate() {

        DBG_MAIN_THREAD

        if ( m_asyncQuery ) {
            m_asyncQuery->terminate();
        }
    }

    // region internet



    // endregion

};

}