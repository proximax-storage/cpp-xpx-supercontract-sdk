/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

namespace sirius::contract {

class Storage {

public:

    virtual ~Storage() = default;

    virtual void synchronizeStorage( const DriveKey& driveKey, const StorageHash& storageHash ) = 0;

//    virtual void cancelStorageSynchronization( const DriveKey& driveKey ) = 0;

    virtual void initiateModifications( const DriveKey& driveKey, uint64_t batchIndex ) = 0;

    virtual void applySandboxStorageModifications( const DriveKey& driveKey, uint64_t batchIndex, bool success ) = 0;

    virtual void evaluateStorageHash( const DriveKey& driveKey, uint64_t batchIndex ) = 0;

    virtual void applyStorageModifications( const DriveKey& driveKey, uint64_t batchIndex, bool success ) = 0;
};

}