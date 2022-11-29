/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "AbsolutePathTag.h"
#include "ApplySandboxStorageModificationsTag.h"
#include "ApplyStorageModificationsTag.h"
#include "CloseFileTag.h"
#include "CreateDirectoriesTag.h"
#include "DirectoryIteratorCreateTag.h"
#include "DirectoryIteratorDestroyTag.h"
#include "DirectoryIteratorHasNextTag.h"
#include "DirectoryIteratorNextTag.h"
#include "EvaluateStorageHashTag.h"
#include "FilesystemTag.h"
#include "FlushFileTag.h"
#include "InitiateModificationsTag.h"
#include "InitiateSandboxModificationsTag.h"
#include "IsFileTag.h"
#include "MoveFilesystemEntryTag.h"
#include "OpenFileTag.h"
#include "PathExistTag.h"
#include "ReadFileTag.h"
#include "RemoveFilesystemEntryTag.h"
#include "SynchronizeStorageTag.h"
#include "WriteFileTag.h"
#include "storage/RPCTag.h"
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <storage/RPCStorage.h>

namespace sirius::contract::storage {

RPCStorage::RPCStorage(GlobalEnvironment& environment, const std::string& serverAddress)
    : m_environment(environment), m_stub(storageServer::StorageServer::NewStub(
                                      grpc::CreateChannel(serverAddress, grpc::InsecureChannelCredentials()))),
      m_completionQueueThread([this] {
          waitForRPCResponse();
      }) {}

RPCStorage::~RPCStorage() {
    ASSERT(isSingleThread(), m_environment.logger())

    for (auto* pTag : m_activeTags) {
        pTag->cancel();
    }

    m_completionQueue.Shutdown();

    if (m_completionQueueThread.joinable()) {
        m_completionQueueThread.join();
    }
}

void RPCStorage::waitForRPCResponse() {
    ASSERT(!isSingleThread(), m_environment.logger())
    void* pTag;
    bool ok;
    while (m_completionQueue.Next(&pTag, &ok)) {
        auto* pQuery = static_cast<RPCTag*>(pTag);
        ASSERT(m_activeTags.contains(pQuery), m_environment.logger())
        m_activeTags.erase(pQuery);
        pQuery->process(ok);
        delete pQuery;
    }
}

void RPCStorage::synchronizeStorage(const DriveKey& driveKey,
                                    const ModificationId& modificationId,
                                    const StorageHash& storageHash,
                                    std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::SynchronizeStorageRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_modification_identifier(modificationId.toString());
    request.set_storage_hash(storageHash.toString());
    auto* tag = new SynchronizeStorageTag(m_environment, std::move(request), *m_stub, m_completionQueue,
                                          std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::initiateModifications(const DriveKey& driveKey,
                                       const ModificationId& modificationId,
                                       std::shared_ptr<AsyncQueryCallback<void>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::InitModificationsRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_modification_identifier(modificationId.toString());
    auto* tag = new InitiateModificationsTag(m_environment, std::move(request), *m_stub, m_completionQueue,
                                             std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::applySandboxStorageModifications(const DriveKey& driveKey,
                                                  bool success,
                                                  std::shared_ptr<AsyncQueryCallback<SandboxModificationDigest>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::ApplySandboxModificationsRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_success(success);
    auto* tag = new ApplySandboxStorageModificationsTag(m_environment, std::move(request), *m_stub,
                                                        m_completionQueue,
                                                        std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::evaluateStorageHash(const DriveKey& driveKey,
                                     std::shared_ptr<AsyncQueryCallback<StorageState>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::EvaluateStorageHashRequest request;
    request.set_drive_key(driveKey.toString());
    auto* tag = new EvaluateStorageHashTag(m_environment, std::move(request), *m_stub, m_completionQueue,
                                           std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::applyStorageModifications(const DriveKey& driveKey, bool success,
                                           std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::ApplyStorageModificationsRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_success(success);
    auto* tag = new ApplyStorageModificationsTag(m_environment, std::move(request), *m_stub, m_completionQueue,
                                                 std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::initiateSandboxModifications(const DriveKey& driveKey,
                                              std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::InitSandboxRequest request;
    request.set_drive_key(driveKey.toString());
    auto* tag = new InitiateSandboxModificationsTag(m_environment,
                                                    std::move(request),
                                                    *m_stub,
                                                    m_completionQueue,
                                                    std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::openFile(const DriveKey& driveKey, const std::string& path, OpenFileMode mode,
                          std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::OpenFileRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_path(path);
    request.set_mode(storageServer::OpenFileMode(int(mode)));
    auto* tag = new OpenFileTag(m_environment,
                                std::move(request),
                                *m_stub,
                                m_completionQueue,
                                std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::writeFile(const DriveKey& driveKey, uint64_t fileId, const std::vector<uint8_t>& buffer,
                           std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::WriteFileRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_file_id(fileId);
    request.set_buffer(std::string(buffer.begin(), buffer.end()));
    auto* tag = new WriteFileTag(m_environment,
                                 std::move(request),
                                 *m_stub,
                                 m_completionQueue,
                                 std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::readFile(const DriveKey& driveKey, uint64_t fileId, uint64_t bytesToRead,
                          std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::ReadFileRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_file_id(fileId);
    request.set_bytes(bytesToRead);
    auto* tag = new ReadFileTag(m_environment,
                                std::move(request),
                                *m_stub,
                                m_completionQueue,
                                std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::closeFile(const DriveKey& driveKey, uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::CloseFileRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_file_id(fileId);
    auto* tag = new CloseFileTag(m_environment,
                                 std::move(request),
                                 *m_stub,
                                 m_completionQueue,
                                 std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::flush(const DriveKey& driveKey, uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::FlushFileRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_file_id(fileId);
    auto* tag = new FlushFileTag(m_environment,
                                 std::move(request),
                                 *m_stub,
                                 m_completionQueue,
                                 std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::absolutePath(const DriveKey& driveKey,
                              const std::string& relativePath,
                              std::shared_ptr<AsyncQueryCallback<std::string>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::AbsolutePathRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_relative_path(relativePath);
    auto* tag = new AbsolutePathTag(m_environment,
                                    std::move(request),
                                    *m_stub,
                                    m_completionQueue,
                                    std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::filesystem(const DriveKey& driveKey,
                            std::shared_ptr<AsyncQueryCallback<std::unique_ptr<Folder>>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::FilesystemRequest request;
    request.set_drive_key(driveKey.toString());
    auto* tag = new FilesystemTag(m_environment,
                                  std::move(request),
                                  *m_stub,
                                  m_completionQueue,
                                  std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::createDirectories(const DriveKey& driveKey, const std::string& path,
                                   std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::CreateDirectoriesRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_path(path);
    auto* tag = new CreateDirectoriesTag(m_environment,
                                         std::move(request),
                                         *m_stub,
                                         m_completionQueue,
                                         std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::directoryIteratorCreate(const DriveKey& driveKey, const std::string& path, bool recursive,
                                         std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::DirectoryIteratorCreateRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_path(path);
    request.set_recursive(recursive);
    auto* tag = new DirectoryIteratorCreateTag(m_environment,
                                               std::move(request),
                                               *m_stub,
                                               m_completionQueue,
                                               std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::removeFilesystemEntry(const DriveKey& driveKey, const std::string& path,
                                       std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::RemoveFilesystemEntryRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_path(path);
    auto* tag = new RemoveFilesystemEntryTag(m_environment,
                                             std::move(request),
                                             *m_stub,
                                             m_completionQueue,
                                             std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::moveFilesystemEntry(const DriveKey& driveKey, const std::string& src, const std::string& dst,
                                     std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::MoveFilesystemEntryRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_src_path(src);
    request.set_dst_path(dst);
    auto* tag = new MoveFilesystemEntryTag(m_environment,
                                           std::move(request),
                                           *m_stub,
                                           m_completionQueue,
                                           std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::isFile(const DriveKey& driveKey, const std::string& path,
                        std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::IsFileRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_path(path);
    auto* tag = new IsFileTag(m_environment,
                              std::move(request),
                              *m_stub,
                              m_completionQueue,
                              std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::pathExist(const DriveKey& driveKey, const std::string& path,
                           std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::PathExistRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_path(path);
    auto* tag = new PathExistTag(m_environment,
                                 std::move(request),
                                 *m_stub,
                                 m_completionQueue,
                                 std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::directoryIteratorHasNext(const DriveKey& driveKey, uint64_t id,
                                          std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::DirectoryIteratorHasNextRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_id(id);
    auto* tag = new DirectoryIteratorHasNextTag(m_environment,
                                                std::move(request),
                                                *m_stub,
                                                m_completionQueue,
                                                std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::directoryIteratorNext(const DriveKey& driveKey, uint64_t id,
                                       std::shared_ptr<AsyncQueryCallback<std::string>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::DirectoryIteratorNextRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_id(id);
    auto* tag = new DirectoryIteratorNextTag(m_environment,
                                             std::move(request),
                                             *m_stub,
                                             m_completionQueue,
                                             std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::directoryIteratorDestroy(const DriveKey& driveKey, uint64_t id,
                                          std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::DirectoryIteratorDestroyRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_id(id);
    auto* tag = new DirectoryIteratorDestroyTag(m_environment,
                                                std::move(request),
                                                *m_stub,
                                                m_completionQueue,
                                                std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

} // namespace sirius::contract::storage