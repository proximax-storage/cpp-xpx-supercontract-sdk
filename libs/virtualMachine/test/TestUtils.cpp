/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "TestUtils.h"

#include <filesystem>

namespace sirius::contract::vm::test {

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

void StorageObserverMock::getAbsolutePath(const std::string& relativePath,
                                          std::shared_ptr<AsyncQueryCallback<std::string>> callback) const {
    callback->postReply(std::filesystem::absolute(relativePath));
}
}