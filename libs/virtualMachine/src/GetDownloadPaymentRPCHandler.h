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

class GetDownloadPaymentRPCHandler
    : public RPCResponseHandler,
      private SingleThread {

    GlobalEnvironment& m_environment;
    supercontractserver::GetDownloadPayment m_request;
    std::weak_ptr<VirtualMachineBlockchainQueryHandler> m_handler;
    std::shared_ptr<AsyncQueryCallback<supercontractserver::GetDownloadPaymentReturn>> m_callback;
    std::shared_ptr<AsyncQuery> m_query;

public:
    GetDownloadPaymentRPCHandler(GlobalEnvironment& environment,
                        const supercontractserver::GetDownloadPayment& request,
                        std::weak_ptr<VirtualMachineBlockchainQueryHandler> handler,
                        std::shared_ptr<AsyncQueryCallback<supercontractserver::GetDownloadPaymentReturn>> callback);

    void process() override;

private:
    void onResult(const expected<uint64_t>& res);
};

} // namespace sirius::contract::vm