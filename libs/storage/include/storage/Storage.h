/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/AsyncQuery.h"
#include "storage/StorageRequests.h"

#include <vector>
#include "Folder.h"

namespace sirius::contract::storage {

enum class OpenFileMode {
    READ,
    WRITE
};

class Storage {

public:

    virtual ~Storage() = default;

    virtual void synchronizeStorage(const DriveKey& driveKey, const StorageHash& storageHash,
                                    std::shared_ptr<AsyncQueryCallback<bool>> callback) = 0;

    virtual void
    initiateModifications(const DriveKey& driveKey,
                          std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void initiateSandboxModifications(const DriveKey& driveKey,
                                              std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void openFile(const DriveKey& driveKey, const std::string& path, OpenFileMode mode,
                          std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) = 0;

    virtual void writeFile(const DriveKey& driveKey, uint64_t fileId, const std::vector<uint8_t>& buffer,
                           std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void readFile(const DriveKey& driveKey, uint64_t fileId, uint64_t bytesToRead,
                          std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) = 0;

    virtual void closeFile(const DriveKey&, uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void flush(const DriveKey&, uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void applySandboxStorageModifications(const DriveKey& driveKey,
                                                  bool success,
                                                  std::shared_ptr<AsyncQueryCallback<SandboxModificationDigest>> callback) = 0;

    virtual void
    evaluateStorageHash(const DriveKey& driveKey,
                        std::shared_ptr<AsyncQueryCallback<StorageState>> callback) = 0;

    virtual void applyStorageModifications(const DriveKey& driveKey, bool success,
                                           std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void absolutePath(const DriveKey& key, const std::string& relativePath,
                              std::shared_ptr<AsyncQueryCallback<std::string>> callback) = 0;

    virtual void
    filesystem(const DriveKey& key, std::shared_ptr<AsyncQueryCallback<std::unique_ptr<Folder>>> callback) = 0;

    virtual void
    createDirectories(const DriveKey& driveKey, const std::string& path,
                      std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void directoryIteratorCreate(const DriveKey& driveKey, const std::string& path, bool recursive,
                                         std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) = 0;

    virtual void directoryIteratorHasNext(const DriveKey& driveKey, uint64_t id,
                                          std::shared_ptr<AsyncQueryCallback<bool>> callback) = 0;

    virtual void directoryIteratorNext(const DriveKey& driveKey, uint64_t id,
                                       std::shared_ptr<AsyncQueryCallback<std::string>> callback) = 0;

    virtual void directoryIteratorDestroy(const DriveKey& driveKey, uint64_t id,
                                          std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void removeFilesystemEntry(const DriveKey& driveKey, const std::string& path,
                                       std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void moveFilesystemEntry(const DriveKey& driveKey, const std::string& src, const std::string& dst,
                                     std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;
};

}