/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <deque>
#include <list>
#include <vector>

#include "contract/Requests.h"

#include "eventHandlers/ContractVirtualMachineEventHandler.h"

namespace sirius::contract {

struct Batch {
    const uint64_t m_batchIndex;
    std::deque<CallRequest> m_callRequests;
};

class BaseBatchesManager: public ContractVirtualMachineEventHandler {

public:

    virtual ~BaseBatchesManager() = default;

    virtual void addCall(const CallRequest&) = 0;

    virtual void onBlockPublished( const Block& block ) = 0;

    virtual bool hasNextBatch() = 0;

    virtual Batch nextBatch() = 0;

    virtual void setAutomaticExecutionsEnabledSince( std::optional<uint64_t> blockHeight ) = 0;

//    uint64_t batchIndex() const {
//
//        // TODO
//        return 0;
//    }
//    {
//        auto batch = std::move(m_batches.front());
//        m_batches.pop_front();
//        return batch;
//    }
};

}