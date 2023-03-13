/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "StorageModificationMock.h"
#include "SandboxModificationMock.h"
#include "gtest/gtest.h"

namespace sirius::contract::test {

StorageModificationMock::StorageModificationMock(std::shared_ptr<StorageInfo> info)
: m_info(std::move(info)) {}

void StorageModificationMock::initiateSandboxModification(
        std::shared_ptr<AsyncQueryCallback<std::unique_ptr<storage::SandboxModification>>> callback) {
    ASSERT_TRUE(m_info->m_actualBatch);
    m_info->m_actualBatch->m_calls.emplace_back();
    callback->postReply(std::make_unique<SandboxModificationMock>(m_info));
}

void StorageModificationMock::evaluateStorageHash(std::shared_ptr<AsyncQueryCallback<storage::StorageState>> callback) {
    ASSERT_TRUE(m_info->m_actualBatch);
    callback->postReply(storage::StorageState(m_info->m_actualBatch->m_lowerSandboxState));
}

void StorageModificationMock::applyStorageModifications(bool success,
                                                        std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT_TRUE(m_info->m_actualBatch);
    if (success) {
        m_info->m_state = m_info->m_actualBatch->m_lowerSandboxState;
    }
    m_info->m_actualBatch->m_success = success;
    m_info->m_historicBatches.emplace_back(std::move(*m_info->m_actualBatch));
    m_info->m_actualBatch.reset();
    callback->postReply(expected<void>());
}

}


