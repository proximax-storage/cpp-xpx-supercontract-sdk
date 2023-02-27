/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/Identifiers.h"

#include <memory>
#include <set>
#include <optional>

namespace sirius::contract::blockchain {

struct PublishedEndBatchExecutionSingleTransactionInfo {
    ContractKey m_contractKey;
    uint64_t    m_batchIndex;
};

}