/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "CallExecutionManager.h"
#include <virtualMachine/VirtualMachineErrorCode.h>

namespace sirius::contract {

CallExecutionManager::CallExecutionManager(ExecutorEnvironment& environment,
                                           std::shared_ptr<vm::VirtualMachineInternetQueryHandler> internetQueryHandler,
                                           std::shared_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                                           std::shared_ptr<vm::VirtualMachineStorageQueryHandler> storageQueryHandler,
                                           std::shared_ptr<AsyncQuery>&& virtualMachineQuery)
        : m_environment(environment)
        , m_internetQueryHandler(std::move(internetQueryHandler))
        , m_blockchainQueryHandler(std::move(blockchainQueryHandler))
        , m_storageQueryHandler(std::move(storageQueryHandler))
        , m_virtualMachineQuery(std::move(virtualMachineQuery)) {}

void CallExecutionManager::run(const vm::CallRequest& callRequest,
                               std::shared_ptr<AsyncQueryCallback<vm::CallExecutionResult>>&& callback) {

    ASSERT(isSingleThread(), m_environment.logger());

    auto virtualMachine = m_environment.virtualMachine().lock();

    if (!virtualMachine) {
        callback->postReply(
                tl::unexpected<std::error_code>(vm::make_error_code(vm::VirtualMachineError::vm_unavailable)));
        return;
    }

    virtualMachine->executeCall(callRequest, m_internetQueryHandler, m_blockchainQueryHandler, m_storageQueryHandler,
                                std::move(callback));
}

std::shared_ptr<vm::VirtualMachineInternetQueryHandler> CallExecutionManager::internetQueryHandler() const {
    return m_internetQueryHandler;
}

std::shared_ptr<vm::VirtualMachineBlockchainQueryHandler> CallExecutionManager::blockchainQueryHandler() const {
    return m_blockchainQueryHandler;
}

std::shared_ptr<vm::VirtualMachineStorageQueryHandler> CallExecutionManager::storageQueryHandler() const {
    return m_storageQueryHandler;
}

}