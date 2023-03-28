/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "LocalLogGuard.h"

#include <string>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/logger.h>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include "spdlog/fmt/fmt.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

#include "logging/ExternalLogger.h"
#include "logging/LoggerConfig.h"
#include <iostream>

#define ASSERT(expr, logger) \
if (!(expr)) {                 \
(logger).critical("ASSERT failed: {} at {}:{}", #expr, __FILE__, __LINE__ ); \
(logger).stop(); \
std::raise(SIGABRT); \
}

namespace sirius::logging {

template<typename... Args>
using format_string_t = fmt::format_string<Args...>;
using string_view_t = fmt::basic_string_view<char>;
using memory_buf_t = fmt::basic_memory_buffer<char, 250>;

class Logger {

private:

    std::string m_prefix;
    std::unique_ptr<ExternalLogger> m_externalLogger;

    // We use shared pointer instead of unique to be able to use atomic operations
    std::shared_ptr<LocalLogGuard> m_guard;
    std::shared_ptr<spdlog::async_logger> m_logger;

public:

    Logger(std::unique_ptr<ExternalLogger>&& externalLogger,
           std::string prefix);

    Logger(const LoggerConfig& loggerConfig,
           std::string prefix);

    Logger(const Logger&) = delete;
    Logger& operator =(const Logger&) = delete;

    Logger(Logger&&) = default;
    Logger& operator =(Logger&&) = default;

    ~Logger();

    void stop();

    template<typename... Args>
    void trace(format_string_t<Args...> fmt, Args&& ... args) {
        if (m_externalLogger) {
            try { m_externalLogger->trace(format(fmt, std::forward<Args>(args)...)); } catch (...) {}
        }
        if (m_logger) {
            try { m_logger->template trace(fmt, std::forward<Args>(args)...); } catch (...) {}
        }
    }

    template<typename... Args>
    void debug(format_string_t<Args...> fmt, Args&& ... args) {
        if (m_externalLogger) {
            try { m_externalLogger->debug(format(fmt, std::forward<Args>(args)...)); } catch (...) {}
        }
        if (m_logger) {
            try { m_logger->template debug(fmt, std::forward<Args>(args)...); } catch (...) {}
        }
    }

    template<typename... Args>
    void info(format_string_t<Args...> fmt, Args&& ... args) {
        if (m_externalLogger) {
            try { m_externalLogger->info(format(fmt, std::forward<Args>(args)...)); } catch (...) {}
        }
        if (m_logger) {
            try { m_logger->template info(fmt, std::forward<Args>(args)...); } catch (...) {}
        }
    }

    template<typename... Args>
    void warn(format_string_t<Args...> fmt, Args&& ... args) {
        if (m_externalLogger) {
            try { m_externalLogger->warn(format(fmt, std::forward<Args>(args)...)); } catch (...) {}
        }
        if (m_logger) {
            try { m_logger->template warn(fmt, std::forward<Args>(args)...); } catch (...) {}
        }
    }

    template<typename... Args>
    void error(format_string_t<Args...> fmt, Args&& ... args) {
        if (m_externalLogger) {
            try { m_externalLogger->err(format(fmt, std::forward<Args>(args)...)); } catch (...) {}
        }
        if (m_logger) {
            try { m_logger->template error(fmt, std::forward<Args>(args)...); } catch (...) {}
        }
    }

    template<typename... Args>
    void critical(format_string_t<Args...> fmt, Args&& ... args) {
        if (m_externalLogger) {
            try { m_externalLogger->critical(format(fmt, std::forward<Args>(args)...)); } catch (...) {}
        }
        if (m_logger) {
            try { m_logger->template critical(fmt, std::forward<Args>(args)...); } catch (...) {}
        }
    }

    template<typename T>
    void trace(const T& msg) {
        if (m_externalLogger) {
            try { m_externalLogger->trace(format("{}", msg)); } catch (...) {}
        }
        if (m_logger) {
            try { m_logger->template trace(msg); } catch (...) {}
        }
    }

    template<typename T>
    void debug(const T& msg) {
        if (m_externalLogger) {
            try { m_externalLogger->debug(format("{}", msg)); } catch (...) {}
        }
        if (m_logger) {
            try { m_logger->template debug(msg); } catch (...) {}
        }
    }

    template<typename T>
    void info(const T& msg) {
        if (m_externalLogger) {
            try { m_externalLogger->info(format("{}", msg)); } catch (...) {}
        }
        if (m_logger) {
            try { m_logger->template info(msg); } catch (...) {}
        }
    }

    template<typename T>
    void warn(const T& msg) {
        if (m_externalLogger) {
            try { m_externalLogger->warn(format("{}", msg)); } catch (...) {}
        }
        if (m_logger) {
            try { m_logger->template warn(msg); } catch (...) {}
        }
    }

    template<typename T>
    void error(const T& msg) {
        if (m_externalLogger) {
            try { m_externalLogger->err(format("{}", msg)); } catch (...) {}
        }
        if (m_logger) {
            try { m_logger->template error(msg); } catch (...) {}
        }
    }

    template<typename T>
    void critical(const T& msg) {
        if (m_externalLogger) {
            try { m_externalLogger->critical(format("{}", msg)); } catch (...) {}
        }
        if (m_logger) {
            try { m_logger->template critical(msg); } catch (...) {}
        }
    }

private:

    template<typename... Args>
    std::string format(string_view_t fmt, Args&& ... args) {
        memory_buf_t buf;
        fmt::detail::vformat_to(buf, fmt, fmt::make_format_args(std::forward<Args>(args)...));
        return "[" + m_prefix + "] " + std::string(buf.begin(), buf.end());
    }


};

}