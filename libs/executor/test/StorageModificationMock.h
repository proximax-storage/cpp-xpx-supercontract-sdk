/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "storage/StorageModification.h"
#include "StorageUtils.h"
#include "utils/Random.h"

namespace sirius::contract::test {

class StorageModificationMock: public storage::StorageModification {

private:

    std::shared_ptr<StorageInfo> m_info;

public:

    StorageModificationMock(std::shared_ptr<StorageInfo> statistics);

    void initiateSandboxModification(
            std::shared_ptr<AsyncQueryCallback<std::unique_ptr<storage::SandboxModification>>> callback) override;

    void evaluateStorageHash(std::shared_ptr<AsyncQueryCallback<storage::StorageState>> callback) override;

    void applyStorageModification(bool success, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

};

}

