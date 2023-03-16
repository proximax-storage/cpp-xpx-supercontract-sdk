/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "FileInfoTag.h"
#include "FilesystemTag.h"
#include "InitiateModificationsTag.h"
#include "SynchronizeStorageTag.h"
#include <grpcpp/security/credentials.h>
#include "RPCStorage.h"
#include "RPCStorageModification.h"
#include "ActualModificationIdTag.h"

namespace sirius::contract::storage {

RPCStorage::RPCStorage(GlobalEnvironment& environment, const std::string& serverAddress)
        : m_environment(environment)
          , m_pRPCClient(std::make_shared<RPCClient>(environment, serverAddress)) {}

void RPCStorage::synchronizeStorage(const DriveKey& driveKey,
                                    const ModificationId& modificationId,
                                    const StorageHash& storageHash,
                                    std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::SynchronizeStorageRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_modification_identifier(modificationId.toString());
    request.set_storage_hash(storageHash.toString());
    auto* tag = new SynchronizeStorageTag(m_environment,
                                          std::move(request),
                                          m_pRPCClient->stub(),
                                          m_pRPCClient->completionQueue(),
                                          std::move(callback));
    m_pRPCClient->addTag(tag);
    tag->start();
}

void RPCStorage::initiateModifications(const DriveKey& driveKey,
                                       const ModificationId& modificationId,
                                       std::shared_ptr<AsyncQueryCallback<std::unique_ptr<StorageModification>>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::InitModificationsRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_modification_identifier(modificationId.toString());

    auto storageModification = std::make_unique<RPCStorageModification>(m_environment, m_pRPCClient, driveKey);
    auto[_, tagCallback] = createAsyncQuery<void>(
            [callback = std::move(callback), storageModification = std::move(storageModification)]
                    (auto&& res) mutable {
                if (!res) {
                    callback->postReply(tl::unexpected<std::error_code>(res.error()));
                    return;
                }
                callback->postReply(std::move(storageModification));
            }, [] {}, m_environment, false, true);
    auto* tag = new InitiateModificationsTag(m_environment,
                                             std::move(request),
                                             m_pRPCClient->stub(),
                                             m_pRPCClient->completionQueue(),
                                             std::move(tagCallback));
    m_pRPCClient->addTag(tag);
    tag->start();
}

void RPCStorage::fileInfo(const DriveKey& driveKey,
                          const std::string& relativePath,
                          std::shared_ptr<AsyncQueryCallback<FileInfo>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::FileInfoRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_relative_path(relativePath);
    auto* tag = new FileInfoTag(m_environment,
                                std::move(request),
                                m_pRPCClient->stub(),
                                m_pRPCClient->completionQueue(),
                                std::move(callback));
    m_pRPCClient->addTag(tag);
    tag->start();
}

void
RPCStorage::actualModificationId(const DriveKey& key, std::shared_ptr<AsyncQueryCallback<ModificationId>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::ActualModificationIdRequest request;
    request.set_drive_key(key.toString());
    auto* tag = new ActualModificationIdTag(m_environment,
                                            std::move(request),
                                            m_pRPCClient->stub(),
                                            m_pRPCClient->completionQueue(),
                                            std::move(callback));
    m_pRPCClient->addTag(tag);
    tag->start();
}

void RPCStorage::filesystem(const DriveKey& driveKey,
                            std::shared_ptr<AsyncQueryCallback<std::unique_ptr<Folder>>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::FilesystemRequest request;
    request.set_drive_key(driveKey.toString());
    auto* tag = new FilesystemTag(m_environment,
                                  std::move(request),
                                  m_pRPCClient->stub(),
                                  m_pRPCClient->completionQueue(),
                                  std::move(callback));
    m_pRPCClient->addTag(tag);
    tag->start();
}

} // namespace sirius::contract::storage