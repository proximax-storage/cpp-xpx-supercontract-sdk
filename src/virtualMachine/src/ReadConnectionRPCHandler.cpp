/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "ReadConnectionRPCHandler.h"

namespace sirius::contract::vm {

ReadConnectionRPCHandler::ReadConnectionRPCHandler(
        GlobalEnvironment& environment,
        const supercontractserver::ReadConnectionStream& request,
        std::weak_ptr<VirtualMachineInternetQueryHandler> handler,
        std::shared_ptr<AsyncQueryCallback<std::optional<supercontractserver::InternetReturnedReadBuffer>>> callback)
        : m_environment(environment)
        , m_request(request)
        , m_handler(std::move(handler))
        , m_callback(std::move(callback)) {}

ReadConnectionRPCHandler::~ReadConnectionRPCHandler() {
    ASSERT(isSingleThread(), m_environment.logger())
    ASSERT(m_query, m_environment.logger())

    m_query->terminate();
}

void ReadConnectionRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Internet Handler Is Absent");
        onResult({});
        return;
    }

    auto callback = createAsyncCallbackAsyncQuery<std::optional<std::vector<uint8_t>>>([this](std::optional<std::vector<uint8_t>>&& result) {
        onResult(result);
    }, [] {}, m_environment);

    handler->read(m_request.identifier(), callback);
}

void ReadConnectionRPCHandler::onResult(const std::optional<std::vector<uint8_t>>& result) {

    ASSERT(isSingleThread(), m_environment.logger())

    supercontractserver::InternetReturnedReadBuffer response;

    if (result.has_value()) {
        response.set_success(true);
        std::string buffer(result->begin(), result->end());
        response.set_buffer(std::move(buffer));
    } else {
        response.set_success(false);
    }
    m_callback->postReply(std::move(response));
}

}
