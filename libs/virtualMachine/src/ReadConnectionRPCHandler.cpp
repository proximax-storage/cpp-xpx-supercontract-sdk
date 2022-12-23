/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "ReadConnectionRPCHandler.h"
#include "virtualMachine/ExecutionErrorConidition.h"
#include <internet/InternetErrorCode.h>

namespace sirius::contract::vm {

ReadConnectionRPCHandler::ReadConnectionRPCHandler(
        GlobalEnvironment& environment,
        const supercontractserver::ReadConnectionStream& request,
        std::weak_ptr<VirtualMachineInternetQueryHandler> handler,
        std::shared_ptr<AsyncQueryCallback<supercontractserver::ReadConnectionStreamReturn>> callback)
        : m_environment(environment)
        , m_request(request)
        , m_handler(std::move(handler))
        , m_callback(std::move(callback)) {}

void ReadConnectionRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Internet Handler Is Absent");
        onResult(tl::make_unexpected(internet::make_error_code(internet::InternetError::internet_unavailable)));
        return;
    }

    auto [query, callback] = createAsyncQuery<std::vector<uint8_t>>([this](auto&& result) {
        onResult(result);
    }, [] {}, m_environment, true, true);

    m_query = std::move(query);

    handler->read(m_request.identifier(), callback);
}

void ReadConnectionRPCHandler::onResult(const expected<std::vector<uint8_t>>& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    if (!res && res.error() == ExecutionError::internet_unavailable) {
        m_callback->postReply(tl::unexpected<std::error_code>(res.error()));
        return;
    }

    supercontractserver::ReadConnectionStreamReturn response;

    if (res.has_value()) {
        response.set_success(true);
        std::string buffer(res->begin(), res->end());
        response.set_buffer(std::move(buffer));
    } else {
        response.set_success(false);
    }
    m_callback->postReply(std::move(response));
}

}
