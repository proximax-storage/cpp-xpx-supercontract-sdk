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

#include "storageServer.grpc.pb.h"

namespace rpc = ::storage;

namespace sirius::contract::storage {

class RPCStorage
        : private SingleThread, public Storage {

private:

    GlobalEnvironment& m_environment;

    std::unique_ptr<rpc::StorageServer::Stub> m_stub;

    grpc::CompletionQueue m_completionQueue;
    std::thread m_completionQueueThread;

public:

    RPCStorage(
            GlobalEnvironment& environment,
            const std::string& serverAddress);

    ~RPCStorage() override;

    void synchronizeStorage(const DriveKey& driveKey, const StorageHash& storageHash,
                            std::shared_ptr<AsyncQueryCallback<std::optional<bool>>> callback) override;

    void initiateModifications(const DriveKey& driveKey,
                               std::shared_ptr<AsyncQueryCallback<std::optional<bool>>> callback) override;

    void applySandboxStorageModifications(const DriveKey& driveKey,
                                          bool success,
                                          std::shared_ptr<AsyncQueryCallback<std::optional<SandboxModificationDigest>>> callback) override;

    void
    evaluateStorageHash(const DriveKey& driveKey,
                        std::shared_ptr<AsyncQueryCallback<std::optional<StorageState>>> callback) override;

    void applyStorageModifications(const DriveKey& driveKey, bool success,
                                   std::shared_ptr<AsyncQueryCallback<std::optional<bool>>> callback) override;

private:

    void waitForRPCResponse();
};

}