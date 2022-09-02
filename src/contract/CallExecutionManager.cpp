/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "CallExecutionManager.h"

namespace sirius::contract {

CallExecutionManager::CallExecutionManager(GlobalEnvironment& environment,
                                           std::weak_ptr<vm::VirtualMachine> virtualMachine,
                                           int repeatTimeout,
                                           const ContractKey& contractKey,
                                           const CallRequest& request,
                                           std::weak_ptr<vm::VirtualMachineInternetQueryHandler> internetQueryHandler,
                                           std::weak_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                                           std::shared_ptr<AsyncQueryCallback<std::optional<vm::CallExecutionResult>>> callback)
        : m_environment(environment)
        , m_virtualMachine(std::move(virtualMachine))
        , m_repeatTimeout(repeatTimeout)
        , m_contractKey(contractKey)
        , m_callRequest(request)
        , m_internetQueryHandler(std::move(internetQueryHandler))
        , m_blockchainQueryHandler(std::move(blockchainQueryHandler))
        , m_callback(std::move(callback)) {
    start();
}

CallExecutionManager::~CallExecutionManager() {
    if (m_virtualMachineQuery) {
        m_virtualMachineQuery->terminate();
    }
}

void CallExecutionManager::start() {

    ASSERT(isSingleThread(), m_environment.logger())

    m_virtualMachineQuery = createAsyncCallbackAsyncQuery<std::optional<vm::CallExecutionResult>>([this] {

    })
}

}