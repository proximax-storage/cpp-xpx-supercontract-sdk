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

class GetTransactionHashRPCHandler
        : public RPCResponseHandler,
          private SingleThread {

    GlobalEnvironment& m_environment;
    supercontractserver::GetTransactionHash m_request;
    std::weak_ptr<VirtualMachineBlockchainQueryHandler> m_handler;
    std::shared_ptr<AsyncQueryCallback<supercontractserver::GetTransactionHashReturn>> m_callback;
    std::shared_ptr<AsyncQuery> m_query;

public:
    GetTransactionHashRPCHandler(GlobalEnvironment& environment,
                           const supercontractserver::GetTransactionHash& request,
                           std::weak_ptr<VirtualMachineBlockchainQueryHandler> handler,
                           std::shared_ptr<AsyncQueryCallback<supercontractserver::GetTransactionHashReturn>> callback);

    void process() override;

private:
    void onResult(const expected<TransactionHash>& res);
};

} // namespace sirius::contract::vm