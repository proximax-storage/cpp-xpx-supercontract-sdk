/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ManualCallBlockchainQueryHandler.h"

namespace sirius::contract {

ManualCallBlockchainQueryHandler::ManualCallBlockchainQueryHandler(
        ExecutorEnvironment& executorEnvironment,
        ContractEnvironment& contractEnvironment,
        const CallerKey& callerKey, uint64_t blockHeight,
        const TransactionHash& transactionHash,
        std::vector<ServicePayment> servicePayments)
        : AutomaticExecutionBlockchainQueryHandler(executorEnvironment, contractEnvironment, callerKey, blockHeight)
        , m_transactionHash(transactionHash)
        , m_servicePayments(std::move(servicePayments)) {}

void ManualCallBlockchainQueryHandler::transactionHash(std::shared_ptr<AsyncQueryCallback<TransactionHash>> callback) {
    callback->postReply(m_transactionHash);
}

void ManualCallBlockchainQueryHandler::servicePayments(
        std::shared_ptr<AsyncQueryCallback<std::vector<ServicePayment>>> callback) {
    callback->postReply(m_servicePayments);
}

}