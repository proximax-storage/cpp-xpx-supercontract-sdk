/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <virtualMachine/VirtualMachineInternetQueryHandler.h>
#include "supercontract_server.pb.h"
#include "supercontract/AsyncQuery.h"
#include "supercontract/SingleThread.h"
#include "RPCResponseHandler.h"

namespace sirius::contract::vm {

class CloseConnectionRPCHandler
        : public RPCResponseHandler, private SingleThread {

    GlobalEnvironment& m_environment;
    supercontractserver::CloseConnection m_request;
    std::weak_ptr<VirtualMachineInternetQueryHandler> m_handler;
    std::shared_ptr<AsyncQueryCallback<std::optional<supercontractserver::CloseConnectionReturn>>> m_callback;
    std::shared_ptr<AsyncQuery> m_query;

public:

    CloseConnectionRPCHandler(GlobalEnvironment& environment,
                              const supercontractserver::CloseConnection& request,
                              std::weak_ptr<VirtualMachineInternetQueryHandler> handler,
                              std::shared_ptr<AsyncQueryCallback<std::optional<supercontractserver::CloseConnectionReturn>>> callback);

    void process() override;

private:

    void onResult(bool success);

};

}