/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "Requests.h"

#include "types.h"

#include <memory>

namespace sirius::contract {

class StorageBridge {

public:

    virtual ~StorageBridge() = default;

    virtual void synchronizeStorage() = 0;

};

}