/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "Block.h"

#include <system_error>
#include <common/Identifiers.h>
#include <common/GlobalEnvironment.h>
#include <common/AsyncQuery.h>

namespace sirius::contract::blockchain {

class Blockchain {

public:

    virtual ~Blockchain() = default;

    virtual void block(uint64_t height, std::shared_ptr<AsyncQueryCallback<Block>> callback) = 0;
};

}