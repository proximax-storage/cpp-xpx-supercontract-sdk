/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "AutorunBlockchainQueryHandler.h"

namespace sirius::contract {

AutorunBlockchainQueryHandler::AutorunBlockchainQueryHandler(ExecutorEnvironment& executorEnvironment,
                                                             ContractEnvironment& contractEnvironment,
                                                             uint64_t height)
        : m_executorEnvironment(executorEnvironment)
        , m_contractEnvironment(contractEnvironment)
        , m_height(height) {}

void AutorunBlockchainQueryHandler::blockHeight(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    callback->postReply(m_height);
}

void AutorunBlockchainQueryHandler::blockHash(std::shared_ptr<AsyncQueryCallback<BlockHash>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto blockchain = m_executorEnvironment.blockchain().lock();
    if (!blockchain) {
        callback->postReply(
                tl::make_unexpected(blockchain::make_error_code(blockchain::BlockchainError::blockchain_unavailable)));
        return;
    }

    auto[asyncQuery, blockchainCallback] = createAsyncQuery<BlockHash>([=, this](auto&& res) {
        m_query.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(tl::make_unexpected(blockchain::make_error_code(blockchain::BlockchainError::blockchain_unavailable)));
    }, m_executorEnvironment, true, true);

    m_query = std::move(asyncQuery);
    blockchain->blockHash(m_height, blockchainCallback);
}

void AutorunBlockchainQueryHandler::blockTime(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto blockchain = m_executorEnvironment.blockchain().lock();
    if (!blockchain) {
        callback->postReply(
                tl::make_unexpected(blockchain::make_error_code(blockchain::BlockchainError::blockchain_unavailable)));
        return;
    }

    auto[asyncQuery, blockchainCallback] = createAsyncQuery<uint64_t>([=, this](auto&& res) {
        m_query.reset();
        callback->postReply(std::forward<decltype(res)>(res));
        }, [=] {
        callback->postReply(tl::make_unexpected(blockchain::make_error_code(blockchain::BlockchainError::blockchain_unavailable)));
        }, m_executorEnvironment, true, true);

    m_query = std::move(asyncQuery);
    blockchain->blockTime(m_height, blockchainCallback);
}

void AutorunBlockchainQueryHandler::blockGenerationTime(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto blockchain = m_executorEnvironment.blockchain().lock();
    if (!blockchain) {
        callback->postReply(
                tl::make_unexpected(blockchain::make_error_code(blockchain::BlockchainError::blockchain_unavailable)));
        return;
    }

    auto[asyncQuery, blockchainCallback] = createAsyncQuery<uint64_t>([=, this](auto&& res) {
        m_query.reset();
        callback->postReply(std::forward<decltype(res)>(res));
        }, [=] {
        callback->postReply(tl::make_unexpected(blockchain::make_error_code(blockchain::BlockchainError::blockchain_unavailable)));
        }, m_executorEnvironment, true, true);

    m_query = std::move(asyncQuery);
    blockchain->blockGenerationTime(m_height, blockchainCallback);
}

}