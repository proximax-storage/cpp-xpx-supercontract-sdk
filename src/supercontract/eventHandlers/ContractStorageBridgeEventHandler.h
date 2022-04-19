/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "contract/StorageQueries.h"

namespace sirius::contract {

class ContractStorageBridgeEventHandler {

public:

    virtual ~ContractStorageBridgeEventHandler() = default;

    //    virtual void onAppliedStorageModifications(const ContractKey& contractKey, )

    virtual bool onStorageSynchronized( uint64_t batchIndex ) {
        return false;
    }

    virtual bool onInitiatedModifications( uint64_t batchIndex ) {
        return false;
    }

    virtual bool onAppliedSandboxStorageModifications( uint64_t batchIndex, bool success,
                                                       int64_t sandboxSizeDelta, int64_t stateSizeDelta ) {
        return false;
    }

    virtual bool onRootHashEvaluated( uint64_t batchIndex, const Hash256& rootHash,
                                      uint64_t usedDriveSize, uint64_t metaFilesSize, uint64_t fileStructureSize ) {
        return false;
    }

    virtual bool onAppliedStorageModifications( uint64_t batchIndex ) {
        return false;
    }
};

}