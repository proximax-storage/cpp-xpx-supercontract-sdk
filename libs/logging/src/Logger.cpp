/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "logging/Logger.h"
#include <mutex>

namespace sirius::logging {

Logger::Logger(std::unique_ptr<ExternalLogger>&& externalLogger, std::string prefix)
        : m_prefix(std::move(prefix))
          , m_externalLogger(std::move(externalLogger)) {}

Logger::Logger(const LoggerConfig& loggerConfig, std::string prefix)
        : m_prefix(std::move(prefix))
          , m_guard(std::make_shared<LocalLogGuard>(8192, 1)) {
    std::vector<spdlog::sink_ptr> sinks;
    if (loggerConfig.logToConsole()) {
        sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    }
    if (loggerConfig.logPath()) {
        sinks.push_back(
                std::make_shared<spdlog::sinks::rotating_file_sink_mt>(*loggerConfig.logPath(),
                                                                       loggerConfig.maxLogSize(),
                                                                       loggerConfig.maxLogFiles()));
    }
    m_logger = std::make_shared<spdlog::async_logger>(m_prefix, sinks.begin(), sinks.end(),
                                                      m_guard->pool(),
                                                      spdlog::async_overflow_policy::block);
    // TODO read log level from config
    m_logger->set_level(spdlog::level::trace);
    m_logger->flush_on(spdlog::level::err);
}

void Logger::stop() {
    std::atomic_store(&m_guard, {});
    if (m_externalLogger) {
        m_externalLogger->stop();
    }
}

Logger::~Logger() {
    stop();
}

}