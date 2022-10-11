/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <memory>
#include <thread>
#include "supercontract/GlobalEnvironment.h"
#include "supercontract/SingleThread.h"
#include "storage/Storage.h"
#include "RPCTag.h"

#include "storageServer.grpc.pb.h"


namespace sirius::contract::storage {

class RPCStorage
        : private SingleThread, public Storage {

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

    void synchronizeStorage(const DriveKey& driveKey, const StorageHash& storageHash,
                            std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void initiateModifications(const DriveKey& driveKey,
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

private:

    void waitForRPCResponse();
};

}