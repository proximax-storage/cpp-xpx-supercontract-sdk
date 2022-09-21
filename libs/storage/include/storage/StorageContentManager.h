/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/AsyncQuery.h"

namespace sirius::contract::storage {

class StorageContentManager {

public:

    virtual ~StorageContentManager() = default;

    virtual void
    getAbsolutePath(const DriveKey& driveKey,
                    const std::string& relativePath,
                    std::shared_ptr<AsyncQueryCallback<std::optional<std::string>>> callback) = 0;

};

}