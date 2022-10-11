/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/AsyncQuery.h"
#include "storage/StorageRequests.h"

#include <vector>

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

};

}