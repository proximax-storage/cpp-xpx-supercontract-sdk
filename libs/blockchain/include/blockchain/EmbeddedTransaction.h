/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <vector>
#include <cstdint>

namespace sirius::contract::blockchain {

struct EmbeddedTransaction {
    uint16_t m_entityType;
    uint32_t m_version;
    std::vector<uint8_t> m_payload;
};

}