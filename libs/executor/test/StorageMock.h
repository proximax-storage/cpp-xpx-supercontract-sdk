/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "storage/Storage.h"
#include "StorageUtils.h"
#include "utils/Random.h"

namespace sirius::contract::test {

class StorageMock: public storage::Storage {

public:

    std::shared_ptr<StorageInfo> m_info;

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
                          std::shared_ptr<AsyncQueryCallback<std::unique_ptr<storage::StorageModification>>> callback) override;

    void absolutePath(const DriveKey& key, const std::string& relativePath,
                              std::shared_ptr<AsyncQueryCallback<std::string>> callback) override;

    void
    filesystem(const DriveKey& key, std::shared_ptr<AsyncQueryCallback<std::unique_ptr<storage::Folder>>> callback) override;

    void
    actualModificationId(const DriveKey& key, std::shared_ptr<AsyncQueryCallback<ModificationId>> callback) override;
};
}


