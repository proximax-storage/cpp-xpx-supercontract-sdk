/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "MoveFileRPCHandler.h"

namespace sirius::contract::vm {

MoveFileRPCHandler::MoveFileRPCHandler(GlobalEnvironment& environment,
                                       const supercontractserver::MoveFile& request,
                                       std::weak_ptr<VirtualMachineStorageQueryHandler> handler,
                                       std::shared_ptr<AsyncQueryCallback<supercontractserver::MoveFileReturn>> callback)
    : m_environment(environment), m_request(request), m_handler(std::move(handler)), m_callback(std::move(callback)) {}

void MoveFileRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Storage Handler Is Absent");
        onResult(tl::make_unexpected(std::make_error_code(std::errc::not_supported)));
        return;
    }

    auto [query, callback] = createAsyncQuery<bool>([this](auto&& res) { onResult(res); }, [] {}, m_environment, true, true);

    m_query = std::move(query);

    handler->moveFile(m_request.old_path(), m_request.new_path(), callback);
}

void MoveFileRPCHandler::onResult(const expected<bool>& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    supercontractserver::MoveFileReturn status;

    if (res.has_value()) {
        status.set_success(*res);
    } else {
        status.set_success(false);
    }

    m_callback->postReply(std::move(status));
}

} // namespace sirius::contract::vm
