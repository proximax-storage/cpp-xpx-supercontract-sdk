/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/StorageQueries.h"

namespace sirius::contract {

class ContractStorageEventHandler {

public:

    virtual ~ContractStorageEventHandler() = default;

    //    virtual void onAppliedStorageModifications(const ContractKey& contractKey, )

    virtual bool onInitiatedModifications( uint64_t batchIndex ) {
        return false;
    }

    virtual bool onAppliedSandboxStorageModifications( uint64_t batchIndex, bool success,
                                                       int64_t sandboxSizeDelta, int64_t stateSizeDelta ) {
        return false;
    }

    virtual bool onStorageHashEvaluated( uint64_t batchIndex, const StorageHash& storageHash,
                                         uint64_t usedDriveSize, uint64_t metaFilesSize, uint64_t fileStructureSize ) {
        return false;
    }
};

}