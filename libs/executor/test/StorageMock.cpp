/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "StorageMock.h"
#include "gtest/gtest.h"

namespace sirius::contract::test {

storage::StorageState nextState(const storage::StorageState& oldState) {
    return utils::generateRandomByteValue<storage::StorageState>(oldState.m_storageHash);
}

storage::StorageState randomState() {
    return utils::generateRandomByteValue<storage::StorageState>();
}

StorageMock::StorageMock() : m_state(randomState()) {}

void StorageMock::synchronizeStorage(const DriveKey& driveKey,
                                     const ModificationId& modificationId,
                                     const StorageHash& storageHash,
                                     std::shared_ptr<AsyncQueryCallback<void>> callback) {
    m_actualBatch.reset();
    m_state.m_storageHash = storageHash;
    callback->postReply(expected<void>());
}

// Storage Modifier virtual functions
void
StorageMock::initiateModifications(const DriveKey& driveKey,
                                   const ModificationId& modificationId,
                                   std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT_FALSE(m_actualBatch);
    m_actualBatch = std::make_optional<BatchModificationStatistics>();
    m_actualBatch->m_lowerSandboxState = m_state;
    callback->postReply(expected<void>());
}

void StorageMock::initiateSandboxModifications(const DriveKey& driveKey,
                                               std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT_TRUE(m_actualBatch);
    m_actualBatch->m_calls.emplace_back();
    callback->postReply(expected<void>());
}

void StorageMock::openFile(const DriveKey& driveKey, const std::string& path, storage::OpenFileMode mode,
                           std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {}

void StorageMock::writeFile(const DriveKey& driveKey, uint64_t fileId, const std::vector<uint8_t>& buffer,
                            std::shared_ptr<AsyncQueryCallback<void>> callback) {}

void StorageMock::readFile(const DriveKey& driveKey, uint64_t fileId, uint64_t bytesToRead,
                           std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {}

void StorageMock::closeFile(const DriveKey&, uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {}

void StorageMock::flush(const DriveKey&, uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {}

void StorageMock::applySandboxStorageModifications(const DriveKey& driveKey,
                                                   bool success,
                                                   std::shared_ptr<AsyncQueryCallback<storage::SandboxModificationDigest>> callback) {
    ASSERT_TRUE(m_actualBatch);
    if (success) {
        m_actualBatch->m_lowerSandboxState = nextState(m_actualBatch->m_lowerSandboxState);
    }
    m_actualBatch->m_calls.back() = success;
    callback->postReply(storage::SandboxModificationDigest{success, 0, 0});
}

void
StorageMock::evaluateStorageHash(const DriveKey& driveKey,
                                 std::shared_ptr<AsyncQueryCallback<storage::StorageState>> callback) {
    ASSERT_TRUE(m_actualBatch);
    callback->postReply(storage::StorageState(m_actualBatch->m_lowerSandboxState));
}

void StorageMock::applyStorageModifications(const DriveKey& driveKey, bool success,
                                            std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT_TRUE(m_actualBatch);
    if (success) {
        m_state = m_actualBatch->m_lowerSandboxState;
    }
    m_actualBatch->m_success = success;
    m_historicBatches.emplace_back(std::move(*m_actualBatch));
    m_actualBatch.reset();
    callback->postReply(expected<void>());
}

void
StorageMock::createDirectories(const DriveKey& driveKey, const std::string& path,
                               std::shared_ptr<AsyncQueryCallback<void>> callback) {}

void StorageMock::directoryIteratorCreate(const DriveKey& driveKey, const std::string& path, bool recursive,
                                          std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {}

void StorageMock::directoryIteratorHasNext(const DriveKey& driveKey, uint64_t id,
                                           std::shared_ptr<AsyncQueryCallback<bool>> callback) {}

void StorageMock::directoryIteratorNext(const DriveKey& driveKey, uint64_t id,
                                        std::shared_ptr<AsyncQueryCallback<std::string>> callback) {}

void StorageMock::directoryIteratorDestroy(const DriveKey& driveKey, uint64_t id,
                                           std::shared_ptr<AsyncQueryCallback<void>> callback) {}

void StorageMock::removeFilesystemEntry(const DriveKey& driveKey, const std::string& path,
                                        std::shared_ptr<AsyncQueryCallback<void>> callback) {}

void StorageMock::moveFilesystemEntry(const DriveKey& driveKey, const std::string& src, const std::string& dst,
                                      std::shared_ptr<AsyncQueryCallback<void>> callback) {}

void StorageMock::pathExist(const DriveKey& driveKey, const std::string& path,
                            std::shared_ptr<AsyncQueryCallback<bool>> callback) {}

void StorageMock::isFile(const DriveKey& driveKey, const std::string& path,
                         std::shared_ptr<AsyncQueryCallback<bool>> callback) {}

// Storage observer virtual functions
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


