/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BatchesManager.h"
#include "BatchesManagerMock.h"


namespace sirius::contract::test {

void BatchesManagerMock::addManualCall(const ManualCallRequest&) {

}

void BatchesManagerMock::addBlock(uint64_t blockHeight) {

}

bool BatchesManagerMock::hasNextBatch() {
    return false;
}

Batch BatchesManagerMock::nextBatch() {
    return Batch();
}

void BatchesManagerMock::setAutomaticExecutionsEnabledSince(const std::optional<uint64_t>& blockHeight) {

}

void BatchesManagerMock::setUnmodifiableUpTo(uint64_t blockHeight) {

}

void BatchesManagerMock::delayBatch(Batch&& batch) {

}

bool BatchesManagerMock::isBatchValid(const Batch& batch) {
    return false;
}

void BatchesManagerMock::cancelBatchesTill(uint64_t batchIndex) {

}

}