/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCTag.h"
#include "storage/Storage.h"
#include "storageServer.grpc.pb.h"
#include <common/GlobalEnvironment.h>
#include <common/SingleThread.h>
#include <memory>
#include <thread>
#include "RPCClient.h"

namespace sirius::contract::storage {

class RPCStorage
    : private SingleThread,
      public Storage {

private:
    GlobalEnvironment& m_environment;

    std::shared_ptr<RPCClient> m_pRPCClient;

public:

    RPCStorage(
        GlobalEnvironment& environment,
        const std::string& serverAddress);

    void synchronizeStorage(const DriveKey& driveKey,
                            const ModificationId& modificationId,
                            const StorageHash& storageHash,
                            std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void initiateModifications(const DriveKey& driveKey,
                               const ModificationId& modificationId,
                               std::shared_ptr<AsyncQueryCallback<std::unique_ptr<StorageModification>>>) override;

    void fileInfo(const DriveKey& key, const std::string& relativePath,
                  std::shared_ptr<AsyncQueryCallback<FileInfo>> callback) override;

    void
    actualModificationId(const DriveKey& key, std::shared_ptr<AsyncQueryCallback<ModificationId>> callback) override;

    void
    filesystem(const DriveKey& key, std::shared_ptr<AsyncQueryCallback<std::unique_ptr<Folder>>> callback) override;
};

} // namespace sirius::contract::storage