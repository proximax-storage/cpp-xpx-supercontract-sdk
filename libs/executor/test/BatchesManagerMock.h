/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <BatchesManager.h>

namespace sirius::contract::test {

class BatchesManagerMock : public BaseBatchesManager {

public:

    void run() override;

    void addManualCall(const ManualCallRequest&) override;

    void addBlock(uint64_t blockHeight) override;

    bool hasNextBatch() override;

    Batch nextBatch() override;

    void setAutomaticExecutionsEnabledSince(const std::optional <uint64_t>& blockHeight) override;

    // Exclusive
    void fixUnmodifiable(uint64_t blockHeight) override;

    void delayBatch(Batch&& batch) override;

    bool isBatchValid(const Batch& batch) override;

    void skipBatches(uint64_t batchIndex) override;

    uint64_t minBatchIndex() override;

};

}