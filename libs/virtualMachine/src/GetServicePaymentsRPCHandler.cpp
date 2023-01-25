/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#include "GetServicePaymentsRPCHandler.h"
#include "virtualMachine/ExecutionErrorConidition.h"
#include <blockchain/BlockchainErrorCode.h>
#include <supercontract/ServicePayment.h>

namespace sirius::contract::vm {

GetServicePaymentsRPCHandler::GetServicePaymentsRPCHandler(GlobalEnvironment& environment,
                                               const supercontractserver::GetServicePayments& request,
                                               std::weak_ptr<VirtualMachineBlockchainQueryHandler> handler,
                                               std::shared_ptr<AsyncQueryCallback<supercontractserver::GetServicePaymentsReturn>> callback)
        : m_environment(environment), m_request(request), m_handler(std::move(handler)), m_callback(
        std::move(callback)) {}

void GetServicePaymentsRPCHandler::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    auto handler = m_handler.lock();

    if (!handler) {
        m_environment.logger().warn("Blockchain Handler Is Absent");
        onResult(tl::make_unexpected(
                blockchain::make_error_code(sirius::contract::blockchain::BlockchainError::blockchain_unavailable)));
        return;
    }

    auto[query, callback] = createAsyncQuery<std::vector<ServicePayment>>([this](auto&& res) { onResult(res); },
                                                       [] {}, m_environment, true, true);

    m_query = std::move(query);

    handler->servicePayments(callback);
}

void GetServicePaymentsRPCHandler::onResult(const expected<std::vector<ServicePayment>>& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    if (!res) {
        m_callback->postReply(tl::unexpected<std::error_code>(res.error()));
        return;
    }

    supercontractserver::GetServicePaymentsReturn status;

    for (const auto& payment: *res) {
        auto* pServicePayment = status.add_payments();
        pServicePayment->set_mosaic_id(payment.m_mosaicId);
        pServicePayment->set_amount(payment.m_amount);
    }

    m_callback->postReply(std::move(status));
}

} // namespace sirius::contract::vm
