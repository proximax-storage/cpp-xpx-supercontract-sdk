/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "RemoveFileIteratorRPCHandler.h"
#include "virtualMachine/ExecutionErrorConidition.h"
#include <storage/StorageErrorCode.h>

namespace sirius::contract::vm {

RemoveFileIteratorRPCHandler::RemoveFileIteratorRPCHandler(GlobalEnvironment& environment,
                                                           const supercontractserver::RemoveDirIterator& request,
                                                           std::weak_ptr<VirtualMachineStorageQueryHandler> handler,
                                                           std::shared_ptr<AsyncQueryCallback<supercontractserver::RemoveDirIteratorReturn>> callback)
    : m_environment(environment), m_request(request), m_handler(std::move(handler)), m_callback(std::move(callback)) {}

void RemoveFileIteratorRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Storage Handler Is Absent");
        onResult(tl::make_unexpected(storage::make_error_code(sirius::contract::storage::StorageError::storage_unavailable)));
        return;
    }

    auto [query, callback] = createAsyncQuery<void>([this](auto&& res) { onResult(res); }, [] {}, m_environment, true, true);

    m_query = std::move(query);

    handler->removeFileIterator(m_request.identifier(), callback);
}

void RemoveFileIteratorRPCHandler::onResult(const expected<void>& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    if (!res && res.error() == ExecutionError::storage_unavailable) {
        m_callback->postReply(tl::unexpected<std::error_code>(res.error()));
        return;
    }

    supercontractserver::RemoveDirIteratorReturn status;

    status.set_success(res.has_value());

    m_callback->postReply(std::move(status));
}

} // namespace sirius::contract::vm
