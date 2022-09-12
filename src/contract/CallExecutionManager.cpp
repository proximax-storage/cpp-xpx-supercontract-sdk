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
    execute();
}

CallExecutionManager::~CallExecutionManager() {
    if (m_virtualMachineQuery) {
        m_virtualMachineQuery->terminate();
    }
}

void CallExecutionManager::execute() {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(!m_virtualMachineQuery, m_environment.logger())

    auto query = createAsyncCallbackAsyncQuery<std::optional<vm::CallExecutionResult>>([this] (auto&& result) {
        if (!result) {
            // We have failed to obtain the result because of some reasons and should try to repeat the effort
            m_environment.logger().warn("Failed To Obtain The Result Of {} Contract Call", m_callRequest.m_callId);
            m_virtualMachineQuery.reset();
            m_repeatTimer = Timer(m_environment.threadManager().context(), m_repeatTimeout, [this] {
                execute();
            });
        }
    }, [] {}, m_environment);
}

}