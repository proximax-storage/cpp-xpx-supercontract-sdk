/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "AsyncQuery.h"

namespace sirius::contract::storage {

struct StorageState {
    StorageHash m_storageHash;
    uint64_t m_usedDriveSize = 0;
    uint64_t m_metaFilesSize = 0;
    uint64_t m_fileStructureSize = 0;
};

struct SandboxModificationDigest {
    bool m_success;
    int64_t m_sandboxSizeDelta;
    int64_t m_stateSizeDelta;
};

class Storage {

public:

    virtual ~Storage() = default;

    virtual void synchronizeStorage(const DriveKey& driveKey, const StorageHash& storageHash) = 0;

//    virtual void cancelStorageSynchronization( const DriveKey& driveKey ) = 0;

    virtual void
    initiateModifications(const DriveKey& driveKey, std::shared_ptr<AsyncQueryCallback<bool>> callback) = 0;

    virtual void applySandboxStorageModifications(const DriveKey& driveKey,
                                                  bool success,
                                                  std::shared_ptr<AsyncQueryCallback<SandboxModificationDigest>> callback) = 0;

    virtual void
    evaluateStorageHash(const DriveKey& driveKey, std::shared_ptr<AsyncQueryCallback<StorageState>> callback) = 0;

    virtual void applyStorageModifications(const DriveKey& driveKey, bool success) = 0;
};

}