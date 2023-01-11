/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "ExecutorEnvironment.h"

#include <virtualMachine/VirtualMachine.h>

namespace sirius::contract {

class CallExecutionManager : private SingleThread {

private:

    ExecutorEnvironment& m_environment;

    std::shared_ptr<vm::VirtualMachineInternetQueryHandler> m_internetQueryHandler;
    std::shared_ptr<vm::VirtualMachineBlockchainQueryHandler> m_blockchainQueryHandler;
    std::shared_ptr<vm::VirtualMachineStorageQueryHandler> m_storageQueryHandler;

    std::shared_ptr<AsyncQuery> m_virtualMachineQuery;

public:

    CallExecutionManager(ExecutorEnvironment& environment,
                         std::shared_ptr<vm::VirtualMachineInternetQueryHandler> internetQueryHandler,
                         std::shared_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                         std::shared_ptr<vm::VirtualMachineStorageQueryHandler> storageQueryHandler,
                         std::shared_ptr<AsyncQuery>&& virtualMachineQuery);

    void run(const vm::CallRequest& callRequest,
             std::shared_ptr<AsyncQueryCallback<vm::CallExecutionResult>>&& callback);

    std::shared_ptr<vm::VirtualMachineInternetQueryHandler> internetQueryHandler() const;

    std::shared_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler() const;

    std::shared_ptr<vm::VirtualMachineStorageQueryHandler> storageQueryHandler() const;

};

}