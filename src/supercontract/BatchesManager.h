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

namespace sirius::contract {

class BaseBatchesManager
        : public ContractBlockchainEventHandler {

public:

    virtual void addManualCall(const CallRequestParameters&) = 0;

    virtual void addBlockInfo( const Block& block ) = 0;

    virtual bool hasNextBatch() = 0;

    virtual Batch nextBatch() = 0;

    virtual void setAutomaticExecutionsEnabledSince( const std::optional<uint64_t>& blockHeight ) = 0;
};

}