/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "virtualMachine/VirtualMachineInternetQueryHandler.h"
#include "supercontract_server.pb.h"
#include "supercontract/AsyncQuery.h"
#include "supercontract/SingleThread.h"
#include "RPCResponseHandler.h"

namespace sirius::contract::vm {

class OpenConnectionRPCHandler
        : public RPCResponseHandler
        , private SingleThread {

    GlobalEnvironment& m_environment;
    supercontractserver::OpenConnection m_request;
    std::weak_ptr<VirtualMachineInternetQueryHandler> m_handler;
    std::shared_ptr<AsyncQueryCallback<std::optional<supercontractserver::OpenConnectionReturn>>> m_callback;
    std::shared_ptr<AsyncQuery> m_query;

public:

    OpenConnectionRPCHandler( GlobalEnvironment& environment,
                                      const supercontractserver::OpenConnection& request,
                                      std::weak_ptr<VirtualMachineInternetQueryHandler> handler,
                                      std::shared_ptr<AsyncQueryCallback<std::optional<supercontractserver::OpenConnectionReturn>>> callback );

    void process() override;

private:

    void onResult( const std::optional<uint64_t>& connectionId );

};

}