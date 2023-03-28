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

class CreateIteratorRPCHandler
    : public RPCResponseHandler,
      private SingleThread {

    GlobalEnvironment& m_environment;
    supercontractserver::CreateDirIterator m_request;
    std::weak_ptr<VirtualMachineStorageQueryHandler> m_handler;
    std::shared_ptr<AsyncQueryCallback<supercontractserver::CreateDirIteratorReturn>> m_callback;
    std::shared_ptr<AsyncQuery> m_query;

public:
    CreateIteratorRPCHandler(GlobalEnvironment& environment,
                             const supercontractserver::CreateDirIterator& request,
                             std::weak_ptr<VirtualMachineStorageQueryHandler> handler,
                             std::shared_ptr<AsyncQueryCallback<supercontractserver::CreateDirIteratorReturn>> callback);

    void process() override;

private:
    void onResult(const expected<uint64_t>& connectionId);
};

} // namespace sirius::contract::vm