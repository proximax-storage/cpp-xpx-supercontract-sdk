/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "TestUtils.h"

namespace sirius::contract::messenger::test {

GlobalEnvironmentMock::GlobalEnvironmentMock()
        : m_logger(getLoggerConfig(), "executor") {}

        ThreadManager& GlobalEnvironmentMock::threadManager() {
    return m_threadManager;
}

logging::Logger& GlobalEnvironmentMock::logger() {
    return m_logger;
}

logging::LoggerConfig GlobalEnvironmentMock::getLoggerConfig() {
    logging::LoggerConfig config;
    config.setLogToConsole(true);
    config.setLogPath({});
    return config;
}
}