/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "CloseConnectionRPCHandler.h"
#include "virtualMachine/ExecutionErrorConidition.h"
#include <internet/InternetErrorCode.h>

namespace sirius::contract::vm {

CloseConnectionRPCHandler::CloseConnectionRPCHandler(
        GlobalEnvironment& environment,
        const supercontractserver::CloseConnection& request,
        std::weak_ptr<VirtualMachineInternetQueryHandler> handler,
        std::shared_ptr<AsyncQueryCallback<supercontractserver::CloseConnectionReturn>> callback)
        : m_environment(environment)
        , m_request(request)
        , m_handler(std::move(handler))
        , m_callback(std::move(callback)) {}

void CloseConnectionRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Internet Handler Is Absent");
        onResult(tl::make_unexpected(internet::make_error_code(internet::InternetError::internet_unavailable)));
        return;
    }

    auto [query, callback] = createAsyncQuery<void>([this](auto&& res) {
        onResult(res);
    }, [] {}, m_environment, true, true);

    m_query = std::move(query);

    handler->closeConnection(m_request.identifier(), callback);
}

void CloseConnectionRPCHandler::onResult(const expected<void>& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    if (!res && res.error() == ExecutionError::internet_unavailable) {
        m_callback->postReply(tl::unexpected<std::error_code>(res.error()));
        return;
    }

    supercontractserver::CloseConnectionReturn status;

    status.set_success(res.has_value());

    m_callback->postReply(std::move(status));
}

}
