/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCResponseHandler.h"
#include <common/AsyncQuery.h>
#include <common/SingleThread.h>
#include "supercontract_server.pb.h"
#include <virtualMachine/VirtualMachineBlockchainQueryHandler.h>

namespace sirius::contract::vm {

class GetServicePaymentsRPCHandler
        : public RPCResponseHandler,
          private SingleThread {

    GlobalEnvironment& m_environment;
    supercontractserver::GetServicePayments m_request;
    std::weak_ptr<VirtualMachineBlockchainQueryHandler> m_handler;
    std::shared_ptr<AsyncQueryCallback<supercontractserver::GetServicePaymentsReturn>> m_callback;
    std::shared_ptr<AsyncQuery> m_query;

public:
    GetServicePaymentsRPCHandler(GlobalEnvironment& environment,
                           const supercontractserver::GetServicePayments& request,
                           std::weak_ptr<VirtualMachineBlockchainQueryHandler> handler,
                           std::shared_ptr<AsyncQueryCallback<supercontractserver::GetServicePaymentsReturn>> callback);

    void process() override;

private:
    void onResult(const expected<std::vector<ServicePayment>>& res);
};

} // namespace sirius::contract::vm