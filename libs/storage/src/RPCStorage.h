/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCTag.h"
#include "storage/Storage.h"
#include "storageServer.grpc.pb.h"
#include "supercontract/GlobalEnvironment.h"
#include "supercontract/SingleThread.h"
#include <memory>
#include <thread>

namespace sirius::contract::storage {

class RPCStorage
    : private SingleThread,
      public Storage {

private:
    GlobalEnvironment& m_environment;

    std::unique_ptr<storageServer::StorageServer::Stub> m_stub;

    grpc::CompletionQueue m_completionQueue;
    std::thread m_completionQueueThread;

    // This set does NOT own the tags
    std::set<RPCTag*> m_activeTags;

public:
    RPCStorage(
        GlobalEnvironment& environment,
        const std::string& serverAddress);

    ~RPCStorage() override;

    void synchronizeStorage(const DriveKey& driveKey,
                            const ModificationId& modificationId,
                            const StorageHash& storageHash,
                            std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void initiateModifications(const DriveKey& driveKey,
                               const ModificationId& modificationId,
                               std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void
    initiateSandboxModifications(const DriveKey& driveKey, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void applySandboxStorageModifications(const DriveKey& driveKey,
                                          bool success,
                                          std::shared_ptr<AsyncQueryCallback<SandboxModificationDigest>> callback) override;

    void
    evaluateStorageHash(const DriveKey& driveKey,
                        std::shared_ptr<AsyncQueryCallback<StorageState>> callback) override;

    void applyStorageModifications(const DriveKey& driveKey, bool success,
                                   std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void openFile(const DriveKey& driveKey, const std::string& path, OpenFileMode mode,
                  std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void writeFile(const DriveKey& driveKey, uint64_t fileId, const std::vector<uint8_t>& buffer,
                   std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void readFile(const DriveKey& driveKey, uint64_t fileId, uint64_t bytesToRead,
                  std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) override;

    void closeFile(const DriveKey& key, uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void flush(const DriveKey& key, uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void absolutePath(const DriveKey& key, const std::string& relativePath,
                      std::shared_ptr<AsyncQueryCallback<std::string>> callback) override;

    void
    filesystem(const DriveKey& key, std::shared_ptr<AsyncQueryCallback<std::unique_ptr<Folder>>> callback) override;

    void createDirectories(const DriveKey& key, const std::string& path,
                           std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void directoryIteratorCreate(const DriveKey& driveKey, const std::string& path, bool recursive,
                                 std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void directoryIteratorHasNext(const DriveKey& driveKey, uint64_t id,
                                  std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void directoryIteratorNext(const DriveKey& driveKey, uint64_t id,
                               std::shared_ptr<AsyncQueryCallback<std::string>> callback) override;

    void directoryIteratorDestroy(const DriveKey& driveKey, uint64_t id,
                                  std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void removeFilesystemEntry(const DriveKey& driveKey, const std::string& path,
                               std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void moveFilesystemEntry(const DriveKey& driveKey, const std::string& src, const std::string& dst,
                             std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void pathExist(const DriveKey& driveKey, const std::string& path,
                   std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void isFile(const DriveKey& driveKey, const std::string& path,
                std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

private:
    void waitForRPCResponse();
};

} // namespace sirius::contract::storage