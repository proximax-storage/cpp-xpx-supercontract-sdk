/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "TestUtils.h"

#include <filesystem>

namespace sirius::contract::vm::test {


logging::LoggerConfig getLoggerConfig() {
    logging::LoggerConfig config;
    config.setLogToConsole(true);
    config.setLogPath({});
    return config;
}

void StorageObserverMock::fileInfo(const DriveKey& driveKey, const std::string& relativePath,
                                   std::shared_ptr<AsyncQueryCallback<storage::FileInfo>> callback) {
    callback->postReply(storage::FileInfo{std::filesystem::absolute(relativePath), 0});
}

void StorageObserverMock::filesystem(const DriveKey& key,
                                           std::shared_ptr<AsyncQueryCallback<std::unique_ptr<storage::Folder>>> callback) {

}

void StorageObserverMock::actualModificationId(const DriveKey& key,
                                               std::shared_ptr<AsyncQueryCallback<ModificationId>> callback) {

}

void StorageObserverMock::synchronizeStorage(const DriveKey& driveKey, const ModificationId& modificationId,
                                             const StorageHash& storageHash,
                                             std::shared_ptr<AsyncQueryCallback<void>> callback) {

}

void StorageObserverMock::initiateModifications(const DriveKey& driveKey, const ModificationId& modificationId,
                                                std::shared_ptr<AsyncQueryCallback<std::unique_ptr<storage::StorageModification>>> callback) {

}
}