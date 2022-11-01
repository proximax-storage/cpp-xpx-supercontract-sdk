/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "NextRPCHandler.h"

namespace sirius::contract::vm {

NextRPCHandler::NextRPCHandler(
    GlobalEnvironment& environment,
    const supercontractserver::NextDirIterator& request,
    std::weak_ptr<VirtualMachineStorageQueryHandler> handler,
    std::shared_ptr<AsyncQueryCallback<supercontractserver::NextDirIteratorReturn>> callback)
    : m_environment(environment), m_request(request), m_handler(std::move(handler)), m_callback(std::move(callback)) {}

void NextRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Storage Handler Is Absent");
        onResult(tl::make_unexpected(std::make_error_code(std::errc::not_supported)));
        return;
    }

    auto [query, callback] = createAsyncQuery<std::vector<uint8_t>>([this](auto&& result) { onResult(result); }, [] {}, m_environment, true, true);

    m_query = std::move(query);

    handler->next(m_request.identifier(), callback);
}

void NextRPCHandler::onResult(const expected<std::vector<uint8_t>>& result) {

    ASSERT(isSingleThread(), m_environment.logger())

    supercontractserver::NextDirIteratorReturn response;

    if (result.has_value()) {
        response.set_success(true);
        std::string buffer(result->begin(), result->end());
        response.set_name(std::move(buffer));
    } else {
        response.set_success(false);
    }
    m_callback->postReply(std::move(response));
}

} // namespace sirius::contract::vm
