/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "OpenConnectionRPCHandler.h"

namespace sirius::contract::vm {

OpenConnectionRPCHandler::OpenConnectionRPCHandler(
        GlobalEnvironment& environment,
        const supercontractserver::OpenConnection& request,
        std::weak_ptr<VirtualMachineInternetQueryHandler> handler,
        std::shared_ptr<AsyncQueryCallback<std::optional<supercontractserver::OpenConnectionReturnStatus>>> callback)
        : m_environment(environment)
        , m_request(request)
        , m_handler(std::move(handler))
        , m_callback(std::move(callback)) {}

OpenConnectionRPCHandler::~OpenConnectionRPCHandler() {
    ASSERT(isSingleThread(), m_environment.logger())
    ASSERT(m_query, m_environment.logger())

    m_query->terminate();
}

void OpenConnectionRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Internet Handler Is Absent");
        onResult({});
        return;
    }

    auto callback = createAsyncCallbackAsyncQuery<std::optional<uint64_t>>([this](std::optional<uint64_t>&& id) {
        onResult(id);
    }, [] {}, m_environment);

    handler->openConnection(m_request.url(), callback);
}

void OpenConnectionRPCHandler::onResult(const std::optional<uint64_t>& connectionId) {

    ASSERT(isSingleThread(), m_environment.logger())

    supercontractserver::OpenConnectionReturnStatus status;

    if (connectionId.has_value()) {
        status.set_success(true);
        status.set_identifier(*connectionId);
    } else {
        status.set_success(false);
    }
    m_callback->postReply(std::move(status));
}

}
