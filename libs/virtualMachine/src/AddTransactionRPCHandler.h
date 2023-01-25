/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCResponseHandler.h"
#include "supercontract/AsyncQuery.h"
#include "supercontract/SingleThread.h"
#include "supercontract_server.pb.h"
#include <virtualMachine/VirtualMachineBlockchainQueryHandler.h>

namespace sirius::contract::vm {

class AddTransactionRPCHandler
        : public RPCResponseHandler,
          private SingleThread {

    GlobalEnvironment& m_environment;
    supercontractserver::AddTransaction m_request;
    std::weak_ptr<VirtualMachineBlockchainQueryHandler> m_handler;
    std::shared_ptr<AsyncQueryCallback<supercontractserver::AddTransactionReturn>> m_callback;
    std::shared_ptr<AsyncQuery> m_query;

public:
    AddTransactionRPCHandler(GlobalEnvironment& environment,
                           const supercontractserver::AddTransaction& request,
                           std::weak_ptr<VirtualMachineBlockchainQueryHandler> handler,
                           std::shared_ptr<AsyncQueryCallback<supercontractserver::AddTransactionReturn>> callback);

    void process() override;

private:
    void onResult(const expected<void>& res);
};

} // namespace sirius::contract::vm