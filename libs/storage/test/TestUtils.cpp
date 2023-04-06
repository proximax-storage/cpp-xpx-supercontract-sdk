/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "TestUtils.h"

#include <filesystem>

namespace sirius::contract::storage::test {

std::optional<std::string> storageAddress() {
#ifdef SIRIUS_CONTRACT_STORAGE_ADDRESS_TEST
    return  SIRIUS_CONTRACT_STORAGE_ADDRESS_TEST;
#else
    return {};
#endif
};

logging::LoggerConfig getLoggerConfig() {
    logging::LoggerConfig config;
    config.setLogToConsole(true);
    config.setLogPath({});
    return config;
}

}