/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "DestroyIteratorRPCHandler.h"

namespace sirius::contract::vm {

DestroyIteratorRPCHandler::DestroyIteratorRPCHandler(GlobalEnvironment& environment,
                                                     const supercontractserver::DestroyDirIterator& request,
                                                     std::weak_ptr<VirtualMachineStorageQueryHandler> handler,
                                                     std::shared_ptr<AsyncQueryCallback<supercontractserver::DestroyDirIteratorReturn>> callback)
    : m_environment(environment), m_request(request), m_handler(std::move(handler)), m_callback(std::move(callback)) {}

void DestroyIteratorRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Storage Handler Is Absent");
        onResult(tl::make_unexpected(std::make_error_code(std::errc::not_supported)));
        return;
    }

    auto [query, callback] = createAsyncQuery<void>([this](auto&& res) { onResult(res); }, [] {}, m_environment, true, true);

    m_query = std::move(query);

    handler->destroyFSIterator(m_request.identifier(), callback);
}

void DestroyIteratorRPCHandler::onResult(const expected<void>& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    supercontractserver::DestroyDirIteratorReturn status;

    status.set_success(res.has_value());

    m_callback->postReply(std::move(status));
}

} // namespace sirius::contract::vm
