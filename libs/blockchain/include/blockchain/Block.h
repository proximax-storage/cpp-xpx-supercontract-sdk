/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <system_error>

#include <supercontract/Identifiers.h>
#include <supercontract/GlobalEnvironment.h>
#include <supercontract/AsyncQuery.h>

namespace sirius::contract::blockchain {

struct Block {
    BlockHash m_blockHash;
    uint64_t m_blockTime;
};

}