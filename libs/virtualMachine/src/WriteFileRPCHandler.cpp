/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "WriteFileRPCHandler.h"
#include "virtualMachine/ExecutionErrorConidition.h"
#include <storage/StorageErrorCode.h>

namespace sirius::contract::vm {

WriteFileRPCHandler::WriteFileRPCHandler(GlobalEnvironment& environment,
                                         const supercontractserver::WriteFileStream& request,
                                         std::weak_ptr<VirtualMachineStorageQueryHandler> handler,
                                         std::shared_ptr<AsyncQueryCallback<supercontractserver::WriteFileStreamReturn>> callback)
    : m_environment(environment), m_request(request), m_handler(std::move(handler)), m_callback(std::move(callback)) {}

void WriteFileRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Storage Handler Is Absent");
        onResult(tl::make_unexpected(storage::make_error_code(sirius::contract::storage::StorageError::storage_unavailable)));
        return;
    }

    auto [query, callback] = createAsyncQuery<uint64_t>([this](auto&& result) { onResult(result); }, [] {}, m_environment, true, true);

    m_query = std::move(query);

    std::vector<uint8_t> buffer(m_request.buffer().begin(), m_request.buffer().end());

    handler->write(m_request.identifier(), buffer, callback);
}

void WriteFileRPCHandler::onResult(const expected<uint64_t>& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    if (!res && res.error() == ExecutionError::storage_unavailable) {
        m_callback->postReply(tl::unexpected<std::error_code>(res.error()));
        return;
    }

    supercontractserver::WriteFileStreamReturn response;

    if (res.has_value()) {
        response.set_success(true);
        response.set_num_bytes_written(*res);
    } else {
        response.set_success(false);
    }
    m_callback->postReply(std::move(response));
}

} // namespace sirius::contract::vm
