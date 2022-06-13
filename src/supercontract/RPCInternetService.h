/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCServerService.h"

namespace sirius::contract {

class InternetService: public RPCServiceImpl<internet::Internet::AsyncService, VirtualMachineInternetQueryHandler> {

public:

    InternetService( const SessionId& sessionId,
                     const std::shared_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineInternetQueryHandler>>& handlersExtractor,
                     ThreadManager& threadManager,
                     const DebugInfo& debugInfo )
                     : RPCServiceImpl( sessionId, handlersExtractor, threadManager, debugInfo ) {
        DBG_MAIN_THREAD
    }

private:

    void registerCalls() override {

        DBG_MAIN_THREAD

        new OpenConnectionRPCInternetRequest( m_sessionId, &m_service, m_completionQueue.get(), m_handlersExtractor, m_terminated, m_dbgInfo );
        new ReadConnectionRPCInternetRequest( m_sessionId, &m_service, m_completionQueue.get(), m_handlersExtractor, m_terminated, m_dbgInfo );
        new CloseConnectionRPCInternetRequest( m_sessionId, &m_service, m_completionQueue.get(), m_handlersExtractor, m_terminated, m_dbgInfo );
    }
};

}