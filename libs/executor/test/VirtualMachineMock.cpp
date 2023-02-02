/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "VirtualMachineMock.h"
#include "virtualMachine/VirtualMachineErrorCode.h"

namespace sirius::contract::test {

VirtualMachineMock::VirtualMachineMock(ThreadManager &threadManager,
                                       std::deque<vm::CallExecutionResult> result,
                                       uint vmFailureNumber)
        : m_threadManager(threadManager)
        , m_result(std::move(result))
        , m_vmFailureNumber(vmFailureNumber) {}

void VirtualMachineMock::executeCall(const vm::CallRequest &request,
                                     std::weak_ptr<vm::VirtualMachineInternetQueryHandler> internetQueryHandler,
                                     std::weak_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                                     std::weak_ptr<vm::VirtualMachineStorageQueryHandler> storageQueryHandler,
                                     std::shared_ptr<AsyncQueryCallback<vm::CallExecutionResult>> callback) {
    if (m_vmFailureNumber > 0) {
        m_vmFailureNumber--;
        m_timers[request.m_callId] = m_threadManager.startTimer(500, [=]() mutable {
            callback->postReply(tl::unexpected<std::error_code>
                    (vm::make_error_code(vm::VirtualMachineError::vm_unavailable)));
        });
        return;
    }

    auto executionResult = m_result.front();
    auto random = rand()%3000;
    m_result.pop_front();
    m_timers[request.m_callId] = m_threadManager.startTimer(random, [=]() mutable {
        callback->postReply(executionResult);
    });
}
}
