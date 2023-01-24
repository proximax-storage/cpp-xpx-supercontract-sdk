/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "AutomaticExecutionBlockchainQueryHandler.h"
#include <blockchain/TransactionBuilder.h>

namespace sirius::contract {

AutomaticExecutionBlockchainQueryHandler::AutomaticExecutionBlockchainQueryHandler(
        ExecutorEnvironment& executorEnvironment,
        ContractEnvironment& contractEnvironment,
        const CallerKey& callerKey,
        uint64_t blockHeight,
        uint64_t executionPayment,
        uint64_t downloadPayment)
        : AutorunBlockchainQueryHandler(executorEnvironment, contractEnvironment, blockHeight)
          , m_caller(callerKey)
          , m_executionPayment(executionPayment)
          , m_downloadPayment(downloadPayment) {}

void AutomaticExecutionBlockchainQueryHandler::callerPublicKey(
        std::shared_ptr<AsyncQueryCallback<CallerKey>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    callback->postReply(m_caller);
}

void AutomaticExecutionBlockchainQueryHandler::downloadPayment(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    callback->postReply(m_downloadPayment);
}

void
AutomaticExecutionBlockchainQueryHandler::executionPayment(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    callback->postReply(m_executionPayment);
}

void
AutomaticExecutionBlockchainQueryHandler::contractPublicKey(std::shared_ptr<AsyncQueryCallback<ContractKey>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    callback->postReply(m_contractEnvironment.contractKey());
}

void AutomaticExecutionBlockchainQueryHandler::addTransaction(std::shared_ptr<AsyncQueryCallback<void>> callback,
                                                              blockchain::EmbeddedTransaction&& embeddedTransaction) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto payload = blockchain::buildEmbeddedTransaction(m_executorEnvironment.executorConfig().networkIdentifier(),
                                                        m_contractEnvironment.contractKey(), embeddedTransaction);
    m_embeddedTransactions.push_back(std::move(payload));

    callback->postReply(expected<void>());
}

}
