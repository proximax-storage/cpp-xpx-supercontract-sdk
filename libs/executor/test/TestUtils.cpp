/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "TestUtils.h"

namespace sirius::contract::test {

logging::LoggerConfig getLoggerConfig() {
    logging::LoggerConfig config;
    config.setLogToConsole(true);
    config.setLogPath({});
    return config;
}

} // namespace sirius::contract::test