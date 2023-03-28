/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "Blockchain.h"
#include <common/SingleThread.h>
#include <common/GlobalEnvironment.h>
#include <utils/NonCopyable.h>

namespace sirius::contract::blockchain {

class CachedBlockchain: public Blockchain, private SingleThread {

private:

    struct BlockchainQuery: public utils::MoveOnly {
        std::shared_ptr<AsyncQuery> m_query;
        std::vector<std::shared_ptr<AsyncQueryCallback<Block>>> m_callbacks;
    };

    GlobalEnvironment& m_environment;

    std::map<uint64_t, Block> m_cache;
    std::shared_ptr<Blockchain> m_blockchain;

    std::map<uint64_t, BlockchainQuery> m_queries;

    uint64_t m_maxCacheSize;

public:

    CachedBlockchain(GlobalEnvironment& environment,
                     std::shared_ptr<Blockchain> blockchain,
                     uint64_t maxCacheSize = 10000);

    void block(uint64_t height, std::shared_ptr<AsyncQueryCallback<Block>> callback) override;

    void addBlock(uint64_t height, const Block& block);

};

}