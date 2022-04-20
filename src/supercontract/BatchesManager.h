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

namespace sirius::contract {

struct Batch {
    const uint64_t m_batchIndex;
    std::deque<CallRequest> m_callRequests;
};

class BaseBatchesManager {
public:

    virtual ~BaseBatchesManager() = default;

    virtual void addCall(const CallRequest&) = 0;

    virtual bool hasFormedBatch() = 0;

    Batch popFormedBatch() {
        if (!hasFormedBatch()) {

        }
        auto&& batch = std::move(m_batches.front());
        m_batches.pop_front();
        return batch;
    }

    void clearOutdatedBatches( uint64_t batchIndex ) {
        while ( m_batches.front().m_batchIndex < batchIndex ) {
            m_batches.pop_front();
        }
    }

private:
    std::deque<Batch> m_batches;
};

class DefaultBatchesManager : BaseBatchesManager{

};

}