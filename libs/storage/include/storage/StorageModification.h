/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <common/AsyncQuery.h>

#include "SandboxModification.h"

#include "StorageRequests.h"

namespace sirius::contract::storage {

class StorageModification {

public:

    virtual ~StorageModification() = default;

    virtual void
    initiateSandboxModification(std::shared_ptr<AsyncQueryCallback<std::unique_ptr<SandboxModification>>> callback) = 0;

    virtual void
    evaluateStorageHash(std::shared_ptr<AsyncQueryCallback<StorageState>> callback) = 0;

    virtual void applyStorageModification(bool success, std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

};

}