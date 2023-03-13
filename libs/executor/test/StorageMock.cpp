/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "StorageMock.h"
#include "gtest/gtest.h"
#include "StorageModificationMock.h"

namespace sirius::contract::test {

StorageMock::StorageMock() : m_info(std::make_shared<StorageInfo>()) {
    m_info->m_state = randomState();
}

void StorageMock::synchronizeStorage(const DriveKey& driveKey,
                                     const ModificationId& modificationId,
                                     const StorageHash& storageHash,
                                     std::shared_ptr<AsyncQueryCallback<void>> callback) {
    m_info->m_actualBatch.reset();
    m_info->m_state.m_storageHash = storageHash;
    callback->postReply(expected<void>());
}

void
StorageMock::initiateModifications(const DriveKey& driveKey,
                                   const ModificationId& modificationId,
                                   std::shared_ptr<AsyncQueryCallback<std::unique_ptr<storage::StorageModification>>> callback) {
    ASSERT_FALSE(m_info->m_actualBatch);
    m_info->m_actualBatch = std::make_optional<BatchModificationStatistics>();
    m_info->m_actualBatch->m_lowerSandboxState = m_info->m_state;
    callback->postReply(std::make_unique<StorageModificationMock>(m_info));
}

void StorageMock::absolutePath(const DriveKey& key, const std::string& relativePath,
                               std::shared_ptr<AsyncQueryCallback<std::string>> callback) {
    callback->postReply(std::filesystem::absolute(relativePath));
}

void
StorageMock::filesystem(const DriveKey& key,
                        std::shared_ptr<AsyncQueryCallback<std::unique_ptr<storage::Folder>>> callback) {}

void
StorageMock::actualModificationId(const DriveKey& key, std::shared_ptr<AsyncQueryCallback<ModificationId>> callback) {

}

}


