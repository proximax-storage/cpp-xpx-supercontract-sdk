/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/GlobalEnvironment.h"
#include <storage/StorageContentManager.h>

namespace sirius::contract::vm::test {

class GlobalEnvironmentMock: public GlobalEnvironment{

private:

    ThreadManager   m_threadManager;
    logging::Logger m_logger;

public:

    GlobalEnvironmentMock();

    ThreadManager& threadManager() override;

    logging::Logger& logger() override;

private:

    logging::LoggerConfig getLoggerConfig();

};

class StorageContentManagerMock: public storage::StorageContentManager {
public:
    void getAbsolutePath(const DriveKey& driveKey, const std::string& relativePath,
                         std::shared_ptr<AsyncQueryCallback<std::string>> callback) override;
};

}