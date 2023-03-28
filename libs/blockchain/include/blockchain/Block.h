/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <system_error>

#include <common/Identifiers.h>
#include <common/GlobalEnvironment.h>
#include <common/AsyncQuery.h>

namespace sirius::contract::blockchain {

struct Block {
    BlockHash m_blockHash;
    uint64_t m_blockTime;
};

}