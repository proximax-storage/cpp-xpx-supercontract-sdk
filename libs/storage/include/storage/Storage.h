/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "StorageModification.h"
#include "Folder.h"
#include <supercontract/Identifiers.h>
#include "StorageModification.h"

namespace sirius::contract::storage {

class Storage {

public:

    virtual ~Storage() = default;

    virtual void synchronizeStorage(const DriveKey& driveKey,
                                    const ModificationId& modificationId,
                                    const StorageHash& storageHash,
                                    std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void
    initiateModifications(const DriveKey& driveKey,
                          const ModificationId& modificationId,
                          std::shared_ptr<AsyncQueryCallback<std::unique_ptr<StorageModification>>> callback) = 0;

    virtual void fileInfo(const DriveKey& key, const std::string& relativePath,
                          std::shared_ptr<AsyncQueryCallback<FileInfo>> callback) = 0;

    virtual void actualModificationId(const DriveKey& key,
                                      std::shared_ptr<AsyncQueryCallback<ModificationId>> callback) = 0;

    virtual void
    filesystem(const DriveKey& key, std::shared_ptr<AsyncQueryCallback<std::unique_ptr<Folder>>> callback) = 0;
};

}