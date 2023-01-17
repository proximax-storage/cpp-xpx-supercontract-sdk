/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <deque>
#include <list>
#include <vector>

#include "Batch.h"

#include "ContractBlockchainEventHandler.h"

#include <blockchain/Block.h>

namespace sirius::contract {

class BaseBatchesManager {

public:

    virtual void addManualCall(const CallRequestParameters&) = 0;

    virtual void addBlockInfo(uint64_t blockHeight, const blockchain::Block& block) = 0;

    virtual bool hasNextBatch() = 0;

    virtual Batch nextBatch() = 0;

    virtual void setAutomaticExecutionsEnabledSince(const std::optional<uint64_t>& blockHeight) = 0;

	// Exclusive
	virtual void setUnmodifiableUpTo(uint64_t blockHeight) = 0;

    virtual void delayBatch(Batch&& batch) = 0;

    virtual void cancelBatchesTill(uint64_t batchIndex) = 0;
};

}