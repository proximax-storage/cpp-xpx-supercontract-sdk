/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <grpcpp/security/credentials.h>
#include <grpcpp/create_channel.h>
#include <storage/RPCStorage.h>
#include "storage/RPCTag.h"
#include "InitiateModificationsTag.h"
#include "ApplySandboxStorageModificationsTag.h"
#include "EvaluateStorageHashTag.h"
#include "SynchronizeStorageTag.h"
#include "ApplyStorageModificationsTag.h"
#include "InitiateSandboxModificationsTag.h"
#include "OpenFileTag.h"
#include "ReadFileTag.h"
#include "WriteFileTag.h"
#include "CloseFileTag.h"
#include "FlushFileTag.h"

namespace sirius::contract::storage {

RPCStorage::RPCStorage(GlobalEnvironment& environment, const std::string& serverAddress)
        : m_environment(environment), m_stub(std::move(storageServer::StorageServer::NewStub(grpc::CreateChannel(
        serverAddress, grpc::InsecureChannelCredentials())))), m_completionQueueThread([this] {
    waitForRPCResponse();
}) {}

RPCStorage::~RPCStorage() {
    ASSERT(isSingleThread(), m_environment.logger())

    for (auto* pTag: m_activeTags) {
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

void RPCStorage::synchronizeStorage(const DriveKey& driveKey, const StorageHash& storageHash,
                                    std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::SynchronizeStorageRequest request;
    request.set_drive_key(driveKey.toString());
    auto* tag = new SynchronizeStorageTag(m_environment, std::move(request), *m_stub, m_completionQueue,
                                          std::move(callback));
    m_activeTags.insert(tag);
    tag->start();
}

void RPCStorage::initiateModifications(const DriveKey& driveKey,
                                       std::shared_ptr<AsyncQueryCallback<void>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    storageServer::InitModificationsRequest request;
    request.set_drive_key(driveKey.toString());
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

void
RPCStorage::evaluateStorageHash(const DriveKey& driveKey,
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

void
RPCStorage::initiateSandboxModifications(const DriveKey& driveKey,
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

void
RPCStorage::closeFile(const DriveKey& driveKey, uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {
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

}