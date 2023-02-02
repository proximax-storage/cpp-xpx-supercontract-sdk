/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "ContractEnvironment.h"
#include "ExecutorEnvironment.h"
#include "virtualMachine/VirtualMachine.h"
#include <storage/Storage.h>
#include <algorithm>
#include <vector>

namespace sirius::contract::test {

template<typename T>
std::vector<size_t> argSort(const std::vector<T>& v) {
    std::vector<size_t> indices(v.size());
    for (size_t i = 0; i < v.size(); i++) {
        indices[i] = i;
    }
    std::sort(indices.begin(), indices.end(),
              [&v](size_t i, size_t j) { return v[i] < v[j]; });
    return indices;
}

class GlobalEnvironmentMock : public GlobalEnvironment {

private:
    ThreadManager m_threadManager;
    logging::Logger m_logger;

public:
    GlobalEnvironmentMock();

    ThreadManager& threadManager() override;

    logging::Logger& logger() override;

private:
    logging::LoggerConfig getLoggerConfig();
};

class StorageObserverMock : public storage::StorageObserver {
public:
    void absolutePath(const DriveKey& driveKey, const std::string& relativePath,
                      std::shared_ptr<AsyncQueryCallback<std::string>> callback) override;

    void
    filesystem(const DriveKey& key, std::shared_ptr<AsyncQueryCallback<std::unique_ptr<storage::Folder>>> callback) override;
};
} // namespace sirius::contract::test
