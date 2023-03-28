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
#include <virtualMachine/VirtualMachineStorageQueryHandler.h>

namespace sirius::contract::vm {

class PathExistRPCHandler
    : public RPCResponseHandler,
      private SingleThread {

    GlobalEnvironment& m_environment;
    supercontractserver::PathExist m_request;
    std::weak_ptr<VirtualMachineStorageQueryHandler> m_handler;
    std::shared_ptr<AsyncQueryCallback<supercontractserver::PathExistReturn>> m_callback;
    std::shared_ptr<AsyncQuery> m_query;

public:
    PathExistRPCHandler(GlobalEnvironment& environment,
                        const supercontractserver::PathExist& request,
                        std::weak_ptr<VirtualMachineStorageQueryHandler> handler,
                        std::shared_ptr<AsyncQueryCallback<supercontractserver::PathExistReturn>> callback);

    void process() override;

private:
    void onResult(const expected<bool>& res);
};

} // namespace sirius::contract::vm