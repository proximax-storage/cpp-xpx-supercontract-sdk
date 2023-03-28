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

class NextRPCHandler
    : public RPCResponseHandler,
      private SingleThread {

    GlobalEnvironment& m_environment;
    supercontractserver::NextDirIterator m_request;
    std::weak_ptr<VirtualMachineStorageQueryHandler> m_handler;
    std::shared_ptr<AsyncQueryCallback<supercontractserver::NextDirIteratorReturn>> m_callback;
    std::shared_ptr<AsyncQuery> m_query;

public:
    NextRPCHandler(GlobalEnvironment& environment,
                   const supercontractserver::NextDirIterator& request,
                   std::weak_ptr<VirtualMachineStorageQueryHandler> handler,
                   std::shared_ptr<AsyncQueryCallback<supercontractserver::NextDirIteratorReturn>> callback);

    void process() override;

private:
    void onResult(const expected<storage::DirectoryIteratorInfo>& result);
};

} // namespace sirius::contract::vm