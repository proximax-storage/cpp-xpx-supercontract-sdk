/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "CloseConnectionRPCHandler.h"

namespace sirius::contract::vm {

CloseConnectionRPCHandler::CloseConnectionRPCHandler(
        GlobalEnvironment& environment,
        const supercontractserver::CloseConnection& request,
        std::weak_ptr<VirtualMachineInternetQueryHandler> handler,
        std::shared_ptr<AsyncQueryCallback<std::optional<supercontractserver::CloseConnectionSuccess>>> callback)
        : m_environment(environment)
        , m_request(request)
        , m_handler(std::move(handler))
        , m_callback(std::move(callback)) {}

void CloseConnectionRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Internet Handler Is Absent");
        onResult({});
        return;
    }

    auto [query, callback] = createAsyncQuery<bool>([this](bool success) {
        onResult(success);
    }, [] {}, m_environment, true, true);

    m_query = std::move(query);

    handler->closeConnection(m_request.identifier(), callback);
}

void CloseConnectionRPCHandler::onResult(bool success) {

    ASSERT(isSingleThread(), m_environment.logger())

    supercontractserver::CloseConnectionSuccess status;

    status.set_success(success);

    m_callback->postReply(std::move(status));
}

}
