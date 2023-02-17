/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/


#include <blockchain/CachedBlockchain.h>
#include <blockchain/BlockchainErrorCode.h>

namespace sirius::contract::blockchain {

CachedBlockchain::CachedBlockchain(GlobalEnvironment& environment, std::shared_ptr<Blockchain> blockchain,
                                   uint64_t maxCacheSize)
                                   : m_environment(environment)
                                   , m_blockchain(std::move(blockchain))
                                   , m_maxCacheSize(maxCacheSize) {}

void CachedBlockchain::addBlock(uint64_t height, const Block& block) {
    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_cache.size() <= m_maxCacheSize, m_environment.logger())

    m_cache.try_emplace(height, block);

    if (m_cache.size() > m_maxCacheSize) {
        m_cache.erase(m_cache.begin());
    }
}

void CachedBlockchain::block(uint64_t height, std::shared_ptr<AsyncQueryCallback<Block>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    auto cacheIt = m_cache.find(height);
    if (cacheIt != m_cache.end()) {
        callback->postReply(cacheIt->second);
        return;
    }

    m_environment.logger().warn("Block not found in blockchain cache. Height: {}", height);

    auto queryIt = m_queries.find(height);

    if (queryIt != m_queries.end()) {
        queryIt->second.m_callbacks.push_back(callback);
        return;
    }

    auto[cacheQuery, cacheCallback] = createAsyncQuery<Block>(
            [this, height](auto&& block) {

                auto queryIt = m_queries.find(height);

                ASSERT(queryIt != m_queries.end(), m_environment.logger())

                addBlock(height, *block);

                for (const auto& callback: queryIt->second.m_callbacks) {
                    callback->postReply(expected<Block>(block));
                }

                m_queries.erase(queryIt);
            }, [this, height] {
                auto queryIt = m_queries.find(height);

                ASSERT(queryIt != m_queries.end(), m_environment.logger())

                for (const auto& callback: queryIt->second.m_callbacks) {
                    callback->postReply(tl::make_unexpected<std::error_code>(
                            make_error_code(BlockchainError::blockchain_unavailable)));
                }

                m_queries.erase(queryIt);
            }, m_environment, true, true);

    BlockchainQuery query;
    query.m_query = std::move(cacheQuery);
    query.m_callbacks.push_back(callback);

    m_queries.try_emplace(height, std::move(query));

    m_blockchain->block(height, callback);
}

}