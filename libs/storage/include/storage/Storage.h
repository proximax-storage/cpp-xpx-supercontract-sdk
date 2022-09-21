/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/AsyncQuery.h"
#include "storage/StorageRequests.h"

namespace sirius::contract::storage {

class Storage {

public:

    virtual ~Storage() = default;

    virtual void synchronizeStorage(const DriveKey& driveKey, const StorageHash& storageHash,
                                    std::shared_ptr<AsyncQueryCallback<std::optional<bool>>> callback) = 0;

    virtual void
    initiateModifications(const DriveKey& driveKey,
                          std::shared_ptr<AsyncQueryCallback<std::optional<bool>>> callback) = 0;

    virtual void applySandboxStorageModifications(const DriveKey& driveKey,
                                                  bool success,
                                                  std::shared_ptr<AsyncQueryCallback<std::optional<SandboxModificationDigest>>> callback) = 0;

    virtual void
    evaluateStorageHash(const DriveKey& driveKey,
                        std::shared_ptr<AsyncQueryCallback<std::optional<StorageState>>> callback) = 0;

    virtual void applyStorageModifications(const DriveKey& driveKey, bool success,
                                           std::shared_ptr<AsyncQueryCallback<std::optional<bool>>> callback) = 0;

    virtual void
    getAbsolutePath(const DriveKey& driveKey, const std::string& relativePath,
                    std::shared_ptr<AsyncQueryCallback<std::optional<std::string>>> callback) = 0;

};

}