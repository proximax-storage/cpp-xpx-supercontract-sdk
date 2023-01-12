/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <system_error>

#include <supercontract/Identifiers.h>

namespace sirius::contract::blockchain {

class Blockchain {

public:

    virtual ~Blockchain() = default;

    virtual BlockHash blockHash(uint64_t height, std::shared_ptr<AsyncQueryCallback<BlockHash>> callback) = 0;

    virtual uint64_t blockTime(uint64_t height, std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) = 0;

    virtual uint64_t blockGenerationTime(uint64_t height, std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) = 0;

};

}