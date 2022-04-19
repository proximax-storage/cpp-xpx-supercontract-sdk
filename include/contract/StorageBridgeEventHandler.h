/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "StorageQueries.h"

namespace sirius::contract {

class StorageBridgeEventHandler {

public:

    virtual ~StorageBridgeEventHandler() = default;

    virtual void onStorageSynchronized( const DriveKey& driveKey, uint64_t batchIndex ) = 0;

    virtual void onInitiatedModifications( const DriveKey& driveKey, uint64_t batchIndex ) = 0;

    virtual void onAppliedSandboxStorageModifications( const DriveKey& driveKey, uint64_t batchIndex, bool success,
                                                       int64_t sandboxSizeDelta, int64_t stateSizeDelta ) = 0;

    virtual void onRootHashEvaluated( const DriveKey& driveKey, uint64_t batchIndex, const Hash256& rootHash, uint64_t usedDriveSize, uint64_t metaFilesSize, uint64_t fileStructureSize ) = 0;

    virtual void onAppliedStorageModifications( const DriveKey& driveKey, uint64_t batchIndex ) = 0;

};

}