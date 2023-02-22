/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "EmbeddedTransaction.h"
#include <supercontract/Identifiers.h>

namespace sirius::contract::blockchain {

using VersionType = uint32_t;
using NetworkIdentifier = uint8_t;

std::vector<uint8_t> buildEmbeddedTransaction(NetworkIdentifier networkIdentifier,
                                              const ContractKey& contractKey,
                                              const EmbeddedTransaction& transaction);

std::pair<Hash256, SerializedAggregatedTransaction> buildAggregatedTransaction(NetworkIdentifier networkIdentifier,
                                                                               const ContractKey& contractKey,
                                                                               const AggregatedTransaction& transaction);

}