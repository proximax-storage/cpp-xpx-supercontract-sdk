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
#include <executor/ManualCallRequest.h>
#include "ContractBlockchainEventHandler.h"

#include <blockchain/Block.h>

namespace sirius::contract {

class BaseBatchesManager {

public:

    virtual ~BaseBatchesManager() = default;

    virtual void run() = 0;

    virtual void addManualCall(const ManualCallRequest&) = 0;

    virtual void addBlock(uint64_t blockHeight) = 0;

    virtual bool hasNextBatch() = 0;

    virtual Batch nextBatch() = 0;

    virtual uint64_t minBatchIndex() = 0;

    virtual void setAutomaticExecutionsEnabledSince(const std::optional<uint64_t>& blockHeight) = 0;
    
	virtual void fixUnmodifiable(uint64_t nextBlockHeight) = 0;

    virtual void delayBatch(Batch&& batch) = 0;

    virtual bool isBatchValid(const Batch& batch) = 0;

    virtual void skipBatches(uint64_t nextBatchIndex) = 0;
};

}