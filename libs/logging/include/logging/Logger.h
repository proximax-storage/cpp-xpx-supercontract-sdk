/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <string>
#include <memory>
#include <mutex>
#include <filesystem>

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

#pragma once

namespace sirius::logging {

// TODO correctly stop logger
#define ASSERT(expr, logger) \
if (!(expr)) {                 \
(logger).critical("ASSERT failed: {} at {}:{}", #expr, __FILE__, __LINE__ ); \
logger.stop();               \
assert(0);                             \
}

template<typename... Args>
using format_string_t = fmt::format_string<Args...>;
using string_view_t = fmt::basic_string_view<char>;
using memory_buf_t = fmt::basic_memory_buffer<char, 250>;

//namespace {
//    inline std::mutex                                    s_creationMutex;
//    inline std::shared_ptr<spdlog::details::thread_pool> s_pThreadPool;
//    inline spdlog::sink_ptr                              s_pConsoleSink;
//}

class LoggerGlobals {
public:
    std::shared_ptr<spdlog::details::thread_pool> m_pThreadPool;
    spdlog::sink_ptr                              m_pConsoleSink;
public:
    static LoggerGlobals& getInstance() {
        static LoggerGlobals instance;
        return instance;
    }

private:

    LoggerGlobals()
    : m_pThreadPool(std::make_shared<spdlog::details::thread_pool>(8192, 1))
    , m_pConsoleSink(std::make_shared<spdlog::sinks::stdout_color_sink_mt>()) {}
    ~LoggerGlobals() = default;

public:

    LoggerGlobals(const LoggerGlobals&)= delete;
    LoggerGlobals& operator=(const LoggerGlobals&)= delete;

};

class Logger {

private:

    std::string                                          m_prefix;
    std::unique_ptr<ExternalLogger>                      m_externalLogger;
    std::shared_ptr<spdlog::async_logger>                m_logger;
    
public:

    Logger( std::unique_ptr<ExternalLogger>&& externalLogger,
            std::string prefix ) : m_prefix( std::move( prefix ) ) {
        if ( !externalLogger ) {
            throw std::runtime_error( "Null External Logger" );
        }
        m_externalLogger = std::move( externalLogger );
        spdlog::thread_pool();
    }

    Logger( const LoggerConfig& loggerConfig,
            std::string prefix ) : m_prefix( std::move( prefix ) ) {
        std::vector<spdlog::sink_ptr> sinks;
        if ( loggerConfig.logToConsole() ) {
            sinks.emplace_back( LoggerGlobals::getInstance().m_pConsoleSink );
        }
        if ( loggerConfig.logPath() ) {
            sinks.push_back(
                    std::make_shared<spdlog::sinks::rotating_file_sink_mt>( *loggerConfig.logPath(),
                                                                            loggerConfig.maxLogSize(),
                                                                            loggerConfig.maxLogFiles() ));
        }
        m_logger = std::make_shared<spdlog::async_logger>( m_prefix, sinks.begin(), sinks.end(), LoggerGlobals::getInstance().m_pThreadPool,
                                                           spdlog::async_overflow_policy::block );
    }

    void stop() {
        LoggerGlobals::getInstance().m_pThreadPool.reset();
        spdlog::shutdown();
    }

    template<typename... Args>
    void trace(format_string_t<Args...> fmt, Args &&... args)
    {
        if ( m_externalLogger ) {
            m_externalLogger->trace(formar(fmt, std::forward<Args>(args)...));
        }
        else {
            m_logger->template trace(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void debug(format_string_t<Args...> fmt, Args &&... args)
    {
        if ( m_externalLogger ) {
            m_externalLogger->debug(format(fmt, std::forward<Args>(args)...));
        }
        else {
            m_logger->template debug(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void info(format_string_t<Args...> fmt, Args &&... args)
    {
        if ( m_externalLogger ) {
            m_externalLogger->info(format(fmt, std::forward<Args>(args)...));
        }
        else {
            m_logger->template info(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void warn(format_string_t<Args...> fmt, Args &&... args)
    {
        if ( m_externalLogger ) {
            m_externalLogger->warn(format(fmt, std::forward<Args>(args)...));
        }
        else {
            m_logger->template warn(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void error(format_string_t<Args...> fmt, Args &&... args)
    {
        if ( m_externalLogger ) {
            m_externalLogger->err(format(fmt, std::forward<Args>(args)...));
        }
        else {
            m_logger->template error(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void critical(format_string_t<Args...> fmt, Args &&... args)
    {
        if ( m_externalLogger ) {
            m_externalLogger->critical(format(fmt, std::forward<Args>(args)...));
        }
        else {
            m_logger->template critical(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename T>
    void trace(const T &msg)
    {
        if ( m_externalLogger ) {
            m_externalLogger->trace(format("{}", msg));
        }
        else {
            m_logger->template trace(msg);
        }
    }

    template<typename T>
    void debug(const T &msg)
    {
        if ( m_externalLogger ) {
            m_externalLogger->debug(format("{}", msg));
        }
        else {
            m_logger->template debug(msg);
        }
    }
    
    template<typename T>
    void info(const T &msg)
    {
        if ( m_externalLogger ) {
            m_externalLogger->info(format("{}", msg));
        }
        else {
            m_logger->template info(msg);
        }
    }

    template<typename T>
    void warn(const T &msg)
    {
        if ( m_externalLogger ) {
            m_externalLogger->warn(format("{}", msg));
        }
        else {
            m_logger->template warn(msg);
        }
    }

    template<typename T>
    void error(const T &msg)
    {
        if ( m_externalLogger ) {
            m_externalLogger->err(format("{}", msg));
        }
        else {
            m_logger->template error(msg);
        }
    }

    template<typename T>
    void critical(const T &msg)
    {
        if ( m_externalLogger ) {
            m_externalLogger->critical(format("{}", msg));
        }
        else {
            m_logger->template critical(msg);
        }
    }

private:
    
    template<typename... Args>
    std::string format( string_view_t fmt, Args &&... args ) {
        memory_buf_t buf;
        fmt::detail::vformat_to(buf, fmt, fmt::make_format_args(std::forward<Args>(args)...));
        return "[" + m_prefix + "] " + std::string(buf.begin(), buf.end());
    }


    
};

}