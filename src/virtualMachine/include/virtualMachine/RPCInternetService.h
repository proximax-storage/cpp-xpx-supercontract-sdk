/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "internet.grpc.pb.h"
#include "RPCServerService.h"
#include "supercontract/GlobalEnvironment.h"
#include "supercontract/SingleThread.h"
#include "VirtualMachineInternetQueryHandler.h"

namespace sirius::contract::vm {

class InternetService:
        public RPCServiceImpl<internet::Internet::AsyncService, VirtualMachineInternetQueryHandler> {

public:

    InternetService( const SessionId& sessionId,
                     const std::shared_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineInternetQueryHandler>>& handlersExtractor,
                     GlobalEnvironment& environment );

private:

    void registerCalls() override;

};

}