/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/GlobalEnvironment.h"

#include "virtualMachine/VirtualMachine.h"

namespace sirius::contract {

class CallExecutionManager: private SingleThread {

private:

    GlobalEnvironment& m_environment;

    std::weak_ptr<vm::VirtualMachine> m_virtualMachine;

    Timer   m_repeatTimer;
    int     m_repeatTimeout;

    CallRequest m_callRequest;

    std::shared_ptr<vm::VirtualMachineInternetQueryHandler> m_internetQueryHandler;
    std::shared_ptr<vm::VirtualMachineBlockchainQueryHandler> m_blockchainQueryHandler;

    std::shared_ptr<AsyncQueryCallback<vm::CallExecutionResult>> m_callback;

    std::shared_ptr<AsyncQuery> m_virtualMachineQuery;

public:

    CallExecutionManager(GlobalEnvironment& environment,
                         std::weak_ptr<vm::VirtualMachine> virtualMachine,
                         int repeatTimeout,
                         const CallRequest& request,
                         std::shared_ptr<vm::VirtualMachineInternetQueryHandler> internetQueryHandler,
                         std::shared_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                         std::shared_ptr<AsyncQueryCallback<vm::CallExecutionResult>> callback);

private:

    void execute();

    void runTimer();

};

}