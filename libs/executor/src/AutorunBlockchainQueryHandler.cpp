/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
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

    auto[asyncQuery, blockchainCallback] = createAsyncQuery<blockchain::Block>([=, this](auto&& res) {
        m_query.reset();
        if (!res) {
            callback->postReply(tl::unexpected<std::error_code>(res.error()));
        }
        callback->postReply(res->m_blockHash);
    }, [=] {
        callback->postReply(
                tl::make_unexpected(blockchain::make_error_code(blockchain::BlockchainError::blockchain_unavailable)));
    }, m_executorEnvironment, true, true);

    m_query = std::move(asyncQuery);
    blockchain->block(m_height, blockchainCallback);
}

void AutorunBlockchainQueryHandler::blockTime(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    blockTime(m_height, callback);
}

void AutorunBlockchainQueryHandler::blockGenerationTime(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_height == 0) {
        callback->postReply(UINT64_MAX);
        return;
    }

    auto[_, previousBlockCallback] = createAsyncQuery<uint64_t>([this, callback](auto&& res) {
        if (!res) {
            callback->postReply(tl::unexpected<std::error_code>(res.error()));
        }

        auto[_, actualBlockCallback] = createAsyncQuery<uint64_t>([callback, prevBlockTime = *res](auto&& res) {
            if (!res) {
                callback->postReply(tl::unexpected<std::error_code>(res.error()));
            }

            callback->postReply(*res - prevBlockTime);

        }, [] {}, m_executorEnvironment, false, false);

        blockTime(m_height, actualBlockCallback);

    }, [] {}, m_executorEnvironment, false, false);

    blockTime(m_height - 1, previousBlockCallback);
}

void AutorunBlockchainQueryHandler::blockTime(uint64_t height,
                                              const std::shared_ptr<AsyncQueryCallback<uint64_t>>& callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto blockchain = m_executorEnvironment.blockchain().lock();
    if (!blockchain) {
        callback->postReply(
                tl::make_unexpected(blockchain::make_error_code(blockchain::BlockchainError::blockchain_unavailable)));
        return;
    }

    auto[asyncQuery, blockchainCallback] = createAsyncQuery<blockchain::Block>([=, this](auto&& res) {
        m_query.reset();
        if (!res) {
            callback->postReply(tl::unexpected<std::error_code>(res.error()));
        }
        callback->postReply(res->m_blockTime);
    }, [=] {
        callback->postReply(
                tl::make_unexpected(blockchain::make_error_code(blockchain::BlockchainError::blockchain_unavailable)));
    }, m_executorEnvironment, true, true);

    m_query = std::move(asyncQuery);
    blockchain->block(height, blockchainCallback);
}

}