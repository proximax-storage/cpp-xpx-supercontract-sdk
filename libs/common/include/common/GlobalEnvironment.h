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

protected:

    ThreadManager m_threadManager;
    std::shared_ptr<logging::Logger> m_logger;

public:

    explicit GlobalEnvironment(std::shared_ptr<logging::Logger> logger);

    virtual ~GlobalEnvironment() = default;

    ThreadManager& threadManager();

    logging::Logger& logger();
};

}