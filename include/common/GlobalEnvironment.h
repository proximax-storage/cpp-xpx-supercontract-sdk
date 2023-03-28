/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#pragma once

#include "logging/Logger.h"
#include <common/ThreadManager.h>

namespace sirius::contract {

class GlobalEnvironment {

public:

    virtual ~GlobalEnvironment() = default;

    virtual ThreadManager& threadManager() = 0;

    virtual logging::Logger& logger() = 0;
};

}