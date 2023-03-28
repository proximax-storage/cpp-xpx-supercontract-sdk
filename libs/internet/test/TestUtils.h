/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <common/GlobalEnvironment.h>

namespace sirius::contract::internet::test {

class GlobalEnvironmentImpl: public GlobalEnvironment{

private:

    ThreadManager   m_threadManager;
    logging::Logger m_logger;

public:

    GlobalEnvironmentImpl() : m_logger(getLoggerConfig(), "executor") {}

    ThreadManager& threadManager() override {
        return m_threadManager;
    }

    logging::Logger& logger() override {
        return m_logger;
    }

private:

    logging::LoggerConfig getLoggerConfig() {
        logging::LoggerConfig config;
        config.setLogToConsole(true);
        config.setLogPath({});
        return config;
    }

};

}