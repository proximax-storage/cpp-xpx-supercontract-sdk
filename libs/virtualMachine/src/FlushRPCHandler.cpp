/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "FlushRPCHandler.h"

namespace sirius::contract::vm {

FlushRPCHandler::FlushRPCHandler(GlobalEnvironment& environment,
                                 const supercontractserver::Flush& request,
                                 std::weak_ptr<VirtualMachineStorageQueryHandler> handler,
                                 std::shared_ptr<AsyncQueryCallback<supercontractserver::FlushReturn>> callback)
    : m_environment(environment), m_request(request), m_handler(std::move(handler)), m_callback(std::move(callback)) {}

void FlushRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Storage Handler Is Absent");
        onResult(tl::make_unexpected(std::make_error_code(std::errc::not_supported)));
        return;
    }

    auto [query, callback] = createAsyncQuery<bool>([this](auto&& res) { onResult(res); }, [] {}, m_environment, true, true);

    m_query = std::move(query);

    handler->flush(m_request.identifier(), callback);
}

void FlushRPCHandler::onResult(const expected<bool>& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    supercontractserver::FlushReturn status;

    if (res.has_value()) {
        status.set_success(*res);
    } else {
        status.set_success(false);
    }

    m_callback->postReply(std::move(status));
}

} // namespace sirius::contract::vm
