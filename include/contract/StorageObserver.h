/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "AsyncQuery.h"

namespace sirius::contract {

class StorageObserver {

public:

    virtual void
    getAbsolutePath( const std::string& relativePath, std::weak_ptr<AbstractAsyncQuery<std::string>> callback) const = 0;

};

}