/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "VirtualMachineMock.h"

namespace sirius::contract::test {

VirtualMachineMock::VirtualMachineMock(ThreadManager &threadManager, std::deque<bool> result)
        : m_threadManager(threadManager), m_result(std::move(result)) {}

void VirtualMachineMock::executeCall(const vm::CallRequest &request,
                                     std::weak_ptr<vm::VirtualMachineInternetQueryHandler> internetQueryHandler,
                                     std::weak_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                                     std::weak_ptr<vm::VirtualMachineStorageQueryHandler> storageQueryHandler,
                                     std::shared_ptr<AsyncQueryCallback<vm::CallExecutionResult>> callback) {
    auto executionResult = m_result.front();
    auto random = rand()%3000;
    m_result.pop_front();
    m_timers[request.m_callId] = m_threadManager.startTimer(random, [=, this]() mutable {
        vm::CallExecutionResult result{
                executionResult,
                0,
                0,
                0,
        };
        callback->postReply(result);
    });
}
}
