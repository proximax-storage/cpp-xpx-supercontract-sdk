/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <grpcpp/security/credentials.h>
#include <grpcpp/create_channel.h>
#include "RPCStorage.h"
#include "RPCTag.h"
#include "InitiateModificationsTag.h"
#include "ApplySandboxStorageModificationsTag.h"
#include "EvaluateStorageHashTag.h"
#include "SynchronizeStorageTag.h"
#include "ApplyStorageModificationsTag.h"

namespace sirius::contract::storage {

RPCStorage::RPCStorage(GlobalEnvironment& environment, const std::string& serverAddress)
        : m_environment(environment)
        , m_stub(std::move(rpc::StorageServer::NewStub(grpc::CreateChannel(
                serverAddress, grpc::InsecureChannelCredentials()))))
        , m_completionQueueThread([this] {
            waitForRPCResponse();
        }) {}

RPCStorage::~RPCStorage() {

    ASSERT(isSingleThread(), m_environment.logger())

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
        pQuery->process(ok);
        delete pQuery;
    }
}

void RPCStorage::synchronizeStorage(const DriveKey& driveKey, const StorageHash& storageHash,
                                    std::shared_ptr<AsyncQueryCallback<std::optional<bool>>> callback) {
    ASSERT(isSingleThread(), m_environment.logger());

    rpc::SynchronizeStorageRequest request;
    request.set_drive_key(driveKey.toString());
    auto* tag = new SynchronizeStorageTag(m_environment, std::move(request), *m_stub, m_completionQueue,
                                          std::move(callback));
    tag->start();
}

void RPCStorage::initiateModifications(const DriveKey& driveKey,
                                       std::shared_ptr<AsyncQueryCallback<std::optional<bool>>> callback) {

    ASSERT(isSingleThread(), m_environment.logger());

    rpc::InitModificationsRequest request;
    request.set_drive_key(driveKey.toString());
    auto* tag = new InitiateModificationsTag(m_environment, std::move(request), *m_stub, m_completionQueue,
                                             std::move(callback));
    tag->start();
}

void RPCStorage::applySandboxStorageModifications(const DriveKey& driveKey,
                                                  bool success,
                                                  std::shared_ptr<AsyncQueryCallback<std::optional<SandboxModificationDigest>>> callback) {

    ASSERT(isSingleThread(), m_environment.logger());

    rpc::ApplySandboxModificationsRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_success(success);
    auto* tag = new ApplySandboxStorageModificationsTag(m_environment, std::move(request), *m_stub, m_completionQueue,
                                                        std::move(callback));
    tag->start();
}

void
RPCStorage::evaluateStorageHash(const DriveKey& driveKey,
                                std::shared_ptr<AsyncQueryCallback<std::optional<StorageState>>> callback) {
    ASSERT(isSingleThread(), m_environment.logger());

    rpc::EvaluateStorageHashRequest request;
    request.set_drive_key(driveKey.toString());
    auto* tag = new EvaluateStorageHashTag(m_environment, std::move(request), *m_stub, m_completionQueue,
                                           std::move(callback));
    tag->start();
}

void RPCStorage::applyStorageModifications(const DriveKey& driveKey, bool success,
                                           std::shared_ptr<AsyncQueryCallback<std::optional<bool>>> callback) {
    ASSERT(isSingleThread(), m_environment.logger());

    rpc::ApplyStorageModificationsRequest request;
    request.set_drive_key(driveKey.toString());
    request.set_success(success);
    auto* tag = new ApplyStorageModificationsTag(m_environment, std::move(request), *m_stub, m_completionQueue,
                                                 std::move(callback));
    tag->start();
}

}