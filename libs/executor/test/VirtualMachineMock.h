/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <deque>
#include "virtualMachine/VirtualMachine.h"

namespace sirius::contract::test {

class VirtualMachineMock : public vm::VirtualMachine {
private:
    ThreadManager& m_threadManager;
    std::deque<vm::CallExecutionResult> m_result;
    std::map<CallId, Timer> m_timers;
    uint m_maxDelay;
    uint m_vmFailureNumber;

public:
    VirtualMachineMock(ThreadManager& threadManager,
                       std::deque<vm::CallExecutionResult> result,
                       uint maxDelay,
                       uint vmFailureNumber = 0);

    void executeCall(const vm::CallRequest& request,
                     std::weak_ptr<vm::VirtualMachineInternetQueryHandler> internetQueryHandler,
                     std::weak_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                     std::weak_ptr<vm::VirtualMachineStorageQueryHandler> storageQueryHandler,
                     std::shared_ptr<AsyncQueryCallback<vm::CallExecutionResult>> callback) override;
};
}