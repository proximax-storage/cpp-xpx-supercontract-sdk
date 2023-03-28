/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <virtualMachine/VirtualMachineInternetQueryHandler.h>
#include "supercontract_server.pb.h"
#include <common/AsyncQuery.h>
#include <common/SingleThread.h>
#include "RPCResponseHandler.h"

namespace sirius::contract::vm {

class ReadConnectionRPCHandler
        : public RPCResponseHandler, private SingleThread {

    GlobalEnvironment& m_environment;
    supercontractserver::ReadConnectionStream m_request;
    std::weak_ptr<VirtualMachineInternetQueryHandler> m_handler;
    std::shared_ptr<AsyncQueryCallback<supercontractserver::ReadConnectionStreamReturn>> m_callback;
    std::shared_ptr<AsyncQuery> m_query;

public:

    ReadConnectionRPCHandler(GlobalEnvironment& environment,
                             const supercontractserver::ReadConnectionStream& request,
                             std::weak_ptr<VirtualMachineInternetQueryHandler> handler,
                             std::shared_ptr<AsyncQueryCallback<supercontractserver::ReadConnectionStreamReturn>> callback);

    void process() override;

private:

    void onResult(const expected<std::vector<uint8_t>>& result);

};

}