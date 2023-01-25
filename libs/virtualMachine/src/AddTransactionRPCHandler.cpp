/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "AddTransactionRPCHandler.h"
#include "virtualMachine/ExecutionErrorConidition.h"
#include <blockchain/BlockchainErrorCode.h>

namespace sirius::contract::vm {

AddTransactionRPCHandler::AddTransactionRPCHandler(GlobalEnvironment& environment,
                                                   const supercontractserver::AddTransaction& request,
                                                   std::weak_ptr<VirtualMachineBlockchainQueryHandler> handler,
                                                   std::shared_ptr<AsyncQueryCallback<supercontractserver::AddTransactionReturn>> callback)
        : m_environment(environment), m_request(request), m_handler(std::move(handler)), m_callback(
        std::move(callback)) {}

void AddTransactionRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Blockchain Handler Is Absent");
        onResult(tl::make_unexpected(
                blockchain::make_error_code(sirius::contract::blockchain::BlockchainError::blockchain_unavailable)));
        return;
    }

    auto[query, callback] = createAsyncQuery<void>([this](auto&& res) { onResult(res); },
                                                   [] {}, m_environment, true, true);

    m_query = std::move(query);

    blockchain::EmbeddedTransaction transaction;
    transaction.m_version = m_request.version();
    transaction.m_entityType = m_request.entity_type();
    transaction.m_payload = {m_request.payload().begin(), m_request.payload().end()};

    handler->addTransaction(std::move(transaction), callback);
}

void AddTransactionRPCHandler::onResult(const expected<void>& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    if (!res) {
        m_callback->postReply(tl::unexpected<std::error_code>(res.error()));
        return;
    }

    supercontractserver::AddTransactionReturn status;

    m_callback->postReply(std::move(status));
}

} // namespace sirius::contract::vm
