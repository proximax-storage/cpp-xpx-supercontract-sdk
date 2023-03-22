/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "RPCSandboxModification.h"
#include "CloseFileTag.h"
#include "CreateDirectoriesTag.h"
#include "DirectoryIteratorCreateTag.h"
#include "DirectoryIteratorDestroyTag.h"
#include "DirectoryIteratorHasNextTag.h"
#include "DirectoryIteratorNextTag.h"
#include "FlushFileTag.h"
#include "IsFileTag.h"
#include "FileSizeTag.h"
#include "MoveFilesystemEntryTag.h"
#include "OpenFileTag.h"
#include "PathExistTag.h"
#include "ReadFileTag.h"
#include "RemoveFilesystemEntryTag.h"
#include "WriteFileTag.h"
#include "ApplySandboxStorageModificationsTag.h"
#include <storage/StorageErrorCode.h>

namespace sirius::contract::storage {

RPCSandboxModification::RPCSandboxModification(GlobalEnvironment& environment,
                                               std::weak_ptr<RPCClient> pRPCClient,
                                               const DriveKey& driveKey)
        : m_environment(environment)
          , m_pRPCClient(std::move(pRPCClient))
          , m_driveKey(driveKey) {}

void RPCSandboxModification::openFile(const std::string& path, OpenFileMode mode,
                                      std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::OpenFileRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_path(path);
    request.set_mode(storageServer::OpenFileMode(int(mode)));
    auto* tag = new OpenFileTag(m_environment,
                                std::move(request),
                                rpcClient->stub(),
                                rpcClient->completionQueue(),
                                std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCSandboxModification::writeFile(uint64_t fileId, const std::vector<uint8_t>& buffer,
                                       std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::WriteFileRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_file_id(fileId);
    request.set_buffer(std::string(buffer.begin(), buffer.end()));
    auto* tag = new WriteFileTag(m_environment,
                                 std::move(request),
                                 rpcClient->stub(),
                                 rpcClient->completionQueue(),
                                 std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCSandboxModification::readFile(uint64_t fileId, uint64_t bytesToRead,
                                      std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::ReadFileRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_file_id(fileId);
    request.set_bytes(bytesToRead);
    auto* tag = new ReadFileTag(m_environment,
                                std::move(request),
                                rpcClient->stub(),
                                rpcClient->completionQueue(),
                                std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void
RPCSandboxModification::closeFile(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::CloseFileRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_file_id(fileId);
    auto* tag = new CloseFileTag(m_environment,
                                 std::move(request),
                                 rpcClient->stub(),
                                 rpcClient->completionQueue(),
                                 std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCSandboxModification::flush(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::FlushFileRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_file_id(fileId);
    auto* tag = new FlushFileTag(m_environment,
                                 std::move(request),
                                 rpcClient->stub(),
                                 rpcClient->completionQueue(),
                                 std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCSandboxModification::createDirectories(const std::string& path,
                                               std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::CreateDirectoriesRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_path(path);
    auto* tag = new CreateDirectoriesTag(m_environment,
                                         std::move(request),
                                         rpcClient->stub(),
                                         rpcClient->completionQueue(),
                                         std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCSandboxModification::directoryIteratorCreate(const std::string& path, bool recursive,
                                                     std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::DirectoryIteratorCreateRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_path(path);
    request.set_recursive(recursive);
    auto* tag = new DirectoryIteratorCreateTag(m_environment,
                                               std::move(request),
                                               rpcClient->stub(),
                                               rpcClient->completionQueue(),
                                               std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCSandboxModification::removeFilesystemEntry(const std::string& path,
                                                   std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::RemoveFilesystemEntryRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_path(path);
    auto* tag = new RemoveFilesystemEntryTag(m_environment,
                                             std::move(request),
                                             rpcClient->stub(),
                                             rpcClient->completionQueue(),
                                             std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCSandboxModification::moveFilesystemEntry(const std::string& src, const std::string& dst,
                                                 std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::MoveFilesystemEntryRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_src_path(src);
    request.set_dst_path(dst);
    auto* tag = new MoveFilesystemEntryTag(m_environment,
                                           std::move(request),
                                           rpcClient->stub(),
                                           rpcClient->completionQueue(),
                                           std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCSandboxModification::isFile(const std::string& path,
                                    std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::IsFileRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_path(path);
    auto* tag = new IsFileTag(m_environment,
                              std::move(request),
                              rpcClient->stub(),
                              rpcClient->completionQueue(),
                              std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCSandboxModification::fileSize(const std::string& path, std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::FileSizeRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_path(path);
    auto* tag = new FileSizeTag(m_environment,
                              std::move(request),
                              rpcClient->stub(),
                              rpcClient->completionQueue(),
                              std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCSandboxModification::pathExist(const std::string& path,
                                       std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::PathExistRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_path(path);
    auto* tag = new PathExistTag(m_environment,
                                 std::move(request),
                                 rpcClient->stub(),
                                 rpcClient->completionQueue(),
                                 std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCSandboxModification::directoryIteratorHasNext(uint64_t id,
                                                      std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::DirectoryIteratorHasNextRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_id(id);
    auto* tag = new DirectoryIteratorHasNextTag(m_environment,
                                                std::move(request),
                                                rpcClient->stub(),
                                                rpcClient->completionQueue(),
                                                std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCSandboxModification::directoryIteratorNext(uint64_t id,
                                                   std::shared_ptr<AsyncQueryCallback<DirectoryIteratorInfo>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::DirectoryIteratorNextRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_id(id);
    auto* tag = new DirectoryIteratorNextTag(m_environment,
                                             std::move(request),
                                             rpcClient->stub(),
                                             rpcClient->completionQueue(),
                                             std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCSandboxModification::directoryIteratorDestroy(uint64_t id,
                                                      std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::DirectoryIteratorDestroyRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_id(id);
    auto* tag = new DirectoryIteratorDestroyTag(m_environment,
                                                std::move(request),
                                                rpcClient->stub(),
                                                rpcClient->completionQueue(),
                                                std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCSandboxModification::applySandboxModification(bool success,
                                                       std::shared_ptr<AsyncQueryCallback<SandboxModificationDigest>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::ApplySandboxModificationsRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_success(success);
    auto* tag = new ApplySandboxStorageModificationsTag(m_environment,
                                                        std::move(request),
                                                        rpcClient->stub(),
                                                        rpcClient->completionQueue(),
                                                        std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

}