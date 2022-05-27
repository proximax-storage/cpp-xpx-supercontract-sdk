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

    InternetService( const std::shared_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineInternetQueryHandler>>& handlersExtractor,
                     ThreadManager& threadManager )
                     : RPCServiceImpl( handlersExtractor, threadManager )
                     {}

private:

    void registerCalls() override {
        new OpenConnectionRPCInternetRequest( &m_service, m_completionQueue.get(), m_handlersExtractor, m_terminated );
    }
};

}