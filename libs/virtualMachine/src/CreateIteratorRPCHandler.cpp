/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "CreateIteratorRPCHandler.h"
#include "common/PlaceHolder.h"

namespace sirius::contract::vm {

CreateIteratorRPCHandler::CreateIteratorRPCHandler(GlobalEnvironment& environment,
                                                   const supercontractserver::CreateDirIterator& request,
                                                   std::weak_ptr<VirtualMachineStorageQueryHandler> handler,
                                                   std::shared_ptr<AsyncQueryCallback<supercontractserver::CreateDirIteratorReturn>> callback)
    : m_environment(environment), m_request(request), m_handler(std::move(handler)), m_callback(std::move(callback)) {}

void CreateIteratorRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Storage Handler Is Absent");
        onResult(tl::make_unexpected(std::make_error_code(sirius::contract::supercontract_error::storage_unavailable)));
        return;
    }

    auto [query, callback] = createAsyncQuery<uint64_t>([this](auto&& id) { onResult(id); }, [] {}, m_environment, true, true);

    m_query = std::move(query);

    handler->createFSIterator(m_request.path(), m_request.recursive(), callback);
}

void CreateIteratorRPCHandler::onResult(const expected<uint64_t>& fileId) {

    ASSERT(isSingleThread(), m_environment.logger())

    supercontractserver::CreateDirIteratorReturn status;

    if (fileId.has_value()) {
        status.set_identifier(*fileId);
    } else {
        status.set_identifier(-1);
    }
    m_callback->postReply(std::move(status));
}

} // namespace sirius::contract::vm
