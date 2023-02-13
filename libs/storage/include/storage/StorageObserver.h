/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/AsyncQuery.h"
#include "Folder.h"

namespace sirius::contract::storage {

class StorageObserver {

public:

    virtual ~StorageObserver() = default;

    virtual void absolutePath(const DriveKey& key, const std::string& relativePath,
                              std::shared_ptr<AsyncQueryCallback<std::string>> callback) = 0;

    virtual void actualModificationId(const DriveKey& key,
                                      std::shared_ptr<AsyncQueryCallback<ModificationId>> callback) = 0;

    virtual void
    filesystem(const DriveKey& key, std::shared_ptr<AsyncQueryCallback<std::unique_ptr<Folder>>> callback) = 0;

};

}