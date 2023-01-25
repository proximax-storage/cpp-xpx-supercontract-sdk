/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "GetExecutionPaymentRPCHandler.h"
#include "virtualMachine/ExecutionErrorConidition.h"
#include <blockchain/BlockchainErrorCode.h>

namespace sirius::contract::vm {

GetExecutionPaymentRPCHandler::GetExecutionPaymentRPCHandler(GlobalEnvironment& environment,
                                                   const supercontractserver::GetExecutionPayment& request,
                                                   std::weak_ptr<VirtualMachineBlockchainQueryHandler> handler,
                                                   std::shared_ptr<AsyncQueryCallback<supercontractserver::GetExecutionPaymentReturn>> callback)
        : m_environment(environment), m_request(request), m_handler(std::move(handler)), m_callback(
        std::move(callback)) {}

void GetExecutionPaymentRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Blockchain Handler Is Absent");
        onResult(tl::make_unexpected(
                blockchain::make_error_code(sirius::contract::blockchain::BlockchainError::blockchain_unavailable)));
        return;
    }

    auto[query, callback] = createAsyncQuery<uint64_t>([this](auto&& res) { onResult(res); },
                                                       [] {}, m_environment, true, true);

    m_query = std::move(query);

    handler->executionPayment(callback);
}

void GetExecutionPaymentRPCHandler::onResult(const expected<uint64_t>& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    if (!res) {
        m_callback->postReply(tl::unexpected<std::error_code>(res.error()));
        return;
    }

    supercontractserver::GetExecutionPaymentReturn status;

    status.set_amount(*res);

    m_callback->postReply(std::move(status));
}

} // namespace sirius::contract::vm
