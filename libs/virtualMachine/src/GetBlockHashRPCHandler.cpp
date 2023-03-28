/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "GetBlockHashRPCHandler.h"
#include "virtualMachine/ExecutionErrorConidition.h"
#include <blockchain/BlockchainErrorCode.h>

namespace sirius::contract::vm {

GetBlockHashRPCHandler::GetBlockHashRPCHandler(GlobalEnvironment& environment,
                                               const supercontractserver::GetBlockHash& request,
                                               std::weak_ptr<VirtualMachineBlockchainQueryHandler> handler,
                                               std::shared_ptr<AsyncQueryCallback<supercontractserver::GetBlockHashReturn>> callback)
        : m_environment(environment), m_request(request), m_handler(std::move(handler)), m_callback(
        std::move(callback)) {}

void GetBlockHashRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Blockchain Handler Is Absent");
        onResult(tl::make_unexpected(
                blockchain::make_error_code(sirius::contract::blockchain::BlockchainError::blockchain_unavailable)));
        return;
    }

    auto[query, callback] = createAsyncQuery<BlockHash>([this](auto&& res) { onResult(res); },
                                                       [] {}, m_environment, true, true);

    m_query = std::move(query);

    handler->blockHash(callback);
}

void GetBlockHashRPCHandler::onResult(const expected<BlockHash>& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    if (!res) {
        m_callback->postReply(tl::unexpected<std::error_code>(res.error()));
        return;
    }

    supercontractserver::GetBlockHashReturn status;

    status.set_hash(std::string(res->begin(), res->end()));

    m_callback->postReply(std::move(status));
}

} // namespace sirius::contract::vm
