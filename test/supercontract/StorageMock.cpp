/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "StorageMock.h"

namespace sirius::contract::test {
void StorageMock::synchronizeStorage(const DriveKey& driveKey,
                                     const ModificationId& modificationId,
                                     const StorageHash& storageHash,
                                     std::shared_ptr<AsyncQueryCallback<bool>> callback) {}

// Storage Modifier virtual functions
void
StorageMock::initiateModifications(const DriveKey& driveKey,
                                   const ModificationId& modificationId,
                                   std::shared_ptr<AsyncQueryCallback<void>> callback) {
        std::cout << "initiateModifications  \n";
        callback->postReply(expected<void>());
}

void StorageMock::initiateSandboxModifications(const DriveKey& driveKey,
                                               std::shared_ptr<AsyncQueryCallback<void>> callback) {
    std::cout << "initiateSandboxModifications  \n";
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
    std::cout << "applySandboxStorageModifications  \n";
    storage::SandboxModificationDigest digest{
            true,
            0,
            0
    };
    callback->postReply(std::move(digest));
}

void
StorageMock::evaluateStorageHash(const DriveKey& driveKey,
                                 std::shared_ptr<AsyncQueryCallback<storage::StorageState>> callback) {
    std::cout << "evaluateStorageHash  \n";
    storage::StorageState state;
    callback->postReply(std::move(state));
}

void StorageMock::applyStorageModifications(const DriveKey& driveKey, bool success,
                                            std::shared_ptr<AsyncQueryCallback<void>> callback) {
    std::cout << "applyStorageModifications  \n";
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
                               std::shared_ptr<AsyncQueryCallback<std::string>> callback) {}

void
StorageMock::filesystem(const DriveKey& key,
                        std::shared_ptr<AsyncQueryCallback<std::unique_ptr<storage::Folder>>> callback) {}
}
