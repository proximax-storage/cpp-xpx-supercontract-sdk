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

//    virtual void onAppliedStorageModifications(const ContractKey& contractKey, )

    virtual void onStorageSynchronized() = 0;

    // Notifies when corresponding modifications are applied (or discarded)
    virtual void onStorageModificationsApplied() = 0;
};

}