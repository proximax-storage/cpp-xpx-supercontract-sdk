/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "storage/Storage.h"
#include "utils/Random.h"

namespace sirius::contract::test {

storage::StorageState nextState(const storage::StorageState& oldState);

storage::StorageState randomState();

struct BatchModificationStatistics {
    std::optional<bool>   m_success;
    storage::StorageState m_lowerSandboxState;
    std::vector<std::optional<bool>> m_calls;
};

class StorageMock: public storage::Storage {

public:

    storage::StorageState m_state;

    std::optional<BatchModificationStatistics> m_actualBatch;
    std::vector<BatchModificationStatistics> m_historicBatches;

public:

    StorageMock();

    // Storage modifier virtual functions
    void synchronizeStorage(const DriveKey& driveKey,
                            const ModificationId& modificationId,
                            const StorageHash& storageHash,
                            std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void
    initiateModifications(const DriveKey& driveKey,
                          const ModificationId& modificationId,
                          std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void initiateSandboxModifications(const DriveKey& driveKey,
                                      std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void openFile(const DriveKey& driveKey, const std::string& path, storage::OpenFileMode mode,
                  std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void writeFile(const DriveKey& driveKey, uint64_t fileId, const std::vector<uint8_t>& buffer,
                   std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void readFile(const DriveKey& driveKey, uint64_t fileId, uint64_t bytesToRead,
                  std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) override;

    void closeFile(const DriveKey&, uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void flush(const DriveKey&, uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void applySandboxStorageModifications(const DriveKey& driveKey,
                                          bool success,
                                          std::shared_ptr<AsyncQueryCallback<storage::SandboxModificationDigest>> callback) override;

    void
    evaluateStorageHash(const DriveKey& driveKey,
                        std::shared_ptr<AsyncQueryCallback<storage::StorageState>> callback) override;

    void applyStorageModifications(const DriveKey& driveKey, bool success,
                                   std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void
    createDirectories(const DriveKey& driveKey, const std::string& path,
                      std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void directoryIteratorCreate(const DriveKey& driveKey, const std::string& path, bool recursive,
                                 std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void directoryIteratorHasNext(const DriveKey& driveKey, uint64_t id,
                                  std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void directoryIteratorNext(const DriveKey& driveKey, uint64_t id,
                               std::shared_ptr<AsyncQueryCallback<std::string>> callback) override;

    void directoryIteratorDestroy(const DriveKey& driveKey, uint64_t id,
                                  std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void removeFilesystemEntry(const DriveKey& driveKey, const std::string& path,
                               std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void moveFilesystemEntry(const DriveKey& driveKey, const std::string& src, const std::string& dst,
                             std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void pathExist(const DriveKey& driveKey, const std::string& path,
                   std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void isFile(const DriveKey& driveKey, const std::string& path,
                std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    // Storage observer virtual functions
    void absolutePath(const DriveKey& key, const std::string& relativePath,
                              std::shared_ptr<AsyncQueryCallback<std::string>> callback) override;

    void
    filesystem(const DriveKey& key, std::shared_ptr<AsyncQueryCallback<std::unique_ptr<storage::Folder>>> callback) override;

    void
    actualModificationId(const DriveKey& key, std::shared_ptr<AsyncQueryCallback<ModificationId>> callback) override;

private:

    void updateState();

};
}


