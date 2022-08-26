/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "virtualMachine/VirtualMachineInternetQueryHandler.h"
#include "virtualMachine/RpcCallResponce.h"
#include "internet.grpc.pb.h"

namespace sirius::contract::vm {

class OpenConnectionRPCInternetRequest
        : public RPCCallResponse<internet::Internet::AsyncService, internet::URL, internet::ReturnStatus, VirtualMachineInternetQueryHandler> {
public:

    OpenConnectionRPCInternetRequest( GlobalEnvironment& environment,
                                      const SessionId& sessionId,
                                      internet::Internet::AsyncService* service,
                                      grpc::ServerCompletionQueue* completionQueue,
                                      std::weak_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineInternetQueryHandler>> handlersExtractor,
                                      const bool& serviceTerminated );

    void process() override;

private:

    void onSuccess( const std::optional<uint64_t>& connectionId );

    void addNextToCompletionQueue();

};

}