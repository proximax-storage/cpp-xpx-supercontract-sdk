/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "TestUtils.h"

#include <filesystem>

namespace sirius::contract::storage::test {

std::optional<std::string> storageAddress() {
#ifdef SIRIUS_CONTRACT_STORAGE_ADDRESS_TEST
    return  SIRIUS_CONTRACT_VM_ADDRESS_TEST;
#else
    return {};
#endif
};

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