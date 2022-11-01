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
#include <virtualMachine/VirtualMachineStorageQueryHandler.h>

namespace sirius::contract::vm {

class WriteFileRPCHandler
    : public RPCResponseHandler,
      private SingleThread {

    GlobalEnvironment& m_environment;
    supercontractserver::WriteFileStream m_request;
    std::weak_ptr<VirtualMachineStorageQueryHandler> m_handler;
    std::shared_ptr<AsyncQueryCallback<supercontractserver::WriteFileStreamReturn>> m_callback;
    std::shared_ptr<AsyncQuery> m_query;

public:
    WriteFileRPCHandler(GlobalEnvironment& environment,
                        const supercontractserver::WriteFileStream& request,
                        std::weak_ptr<VirtualMachineStorageQueryHandler> handler,
                        std::shared_ptr<AsyncQueryCallback<supercontractserver::WriteFileStreamReturn>> callback);

    void process() override;

private:
    void onResult(const expected<uint64_t>& bytesWritten);
};

} // namespace sirius::contract::vm