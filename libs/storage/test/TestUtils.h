/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/GlobalEnvironment.h"
#include <storage/StorageObserver.h>

namespace sirius::contract::storage::test {

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

class StorageObserverMock: public storage::StorageObserver {
public:

    void absolutePath(const DriveKey& key, const std::string& relativePath,
                      std::shared_ptr<AsyncQueryCallback<std::string>> callback) override;

    void
    filesystem(const DriveKey& key, std::shared_ptr<AsyncQueryCallback<std::unique_ptr<Folder>>> callback) override;

};

}