/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <filesystem>
#include <optional>

namespace sirius::logging {

namespace fs = std::filesystem;

class LoggerConfig {

private:

    bool                    m_logToConsole = true;
    std::optional<fs::path> m_logPath;
    int                     m_maxLogSize = 10 * 1024 * 1024;
    int                     m_maxLogFiles = 3;

public:

    bool logToConsole() const {
        return m_logToConsole;
    }

    void setLogToConsole( bool logToConsole ) {
        m_logToConsole = logToConsole;
    }

    const std::optional<fs::path>& logPath() const {
        return m_logPath;
    }

    void setLogPath( const std::optional<fs::path>& logPath ) {
        m_logPath = logPath;
    }

    int maxLogSize() const {
        return m_maxLogSize;
    }

    void setMaxLogSize( int maxLogSize ) {
        m_maxLogSize = maxLogSize;
    }

    int maxLogFiles() const {
        return m_maxLogFiles;
    }

    void setMaxLogFiles( int maxLogFiles ) {
        m_maxLogFiles = maxLogFiles;
    }

};

}