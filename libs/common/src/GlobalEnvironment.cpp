/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <common/GlobalEnvironment.h>

namespace sirius::contract {

GlobalEnvironment::GlobalEnvironment(std::shared_ptr<logging::Logger> logger)
        : m_threadManager(std::make_shared<ThreadManager>())
        , m_logger(std::move(logger)) {}

GlobalEnvironment::GlobalEnvironment(std::shared_ptr<logging::Logger> logger,
                                     std::shared_ptr<ThreadManager> threadManager)
                                     : m_threadManager(std::move(threadManager))
                                     , m_logger(std::move(logger)) {}

ThreadManager& GlobalEnvironment::threadManager() {
    return *m_threadManager;
}

logging::Logger& GlobalEnvironment::logger() {
    return *m_logger;
}

}