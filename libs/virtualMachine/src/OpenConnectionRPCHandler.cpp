/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "OpenConnectionRPCHandler.h"
#include "virtualMachine/ExecutionErrorConidition.h"
#include <internet/InternetErrorCode.h>

namespace sirius::contract::vm {

OpenConnectionRPCHandler::OpenConnectionRPCHandler(
        GlobalEnvironment& environment,
        const supercontractserver::OpenConnection& request,
        std::weak_ptr<VirtualMachineInternetQueryHandler> handler,
        std::shared_ptr<AsyncQueryCallback<supercontractserver::OpenConnectionReturn>> callback)
        : m_environment(environment)
        , m_request(request)
        , m_handler(std::move(handler))
        , m_callback(std::move(callback)) {}

void OpenConnectionRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Internet Handler Is Absent");
        onResult(tl::make_unexpected(internet::make_error_code(internet::InternetError::internet_unavailable)));
        return;
    }

    auto [query, callback] = createAsyncQuery<uint64_t>([this](auto&& id) {
        onResult(id);
    }, [] {}, m_environment, true, true);

    m_query = std::move(query);

    handler->openConnection(m_request.url(), m_request.soft_revocation_mode(), callback);
}

void OpenConnectionRPCHandler::onResult(const expected<uint64_t>& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    supercontractserver::OpenConnectionReturn status;

    if (!res && res.error() == ExecutionError::internet_unavailable) {
        m_callback->postReply(tl::unexpected<std::error_code>(res.error()));
        return;
    }

    if (res.has_value()) {
        status.set_success(true);
        status.set_identifier(*res);
    } else {
        status.set_success(false);
    }
    m_callback->postReply(std::move(status));
}

}
