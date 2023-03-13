/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <storage/StorageErrorCode.h>
#include "RPCStorageModification.h"
#include "InitiateSandboxModificationsTag.h"
#include "RPCSandboxModification.h"
#include "EvaluateStorageHashTag.h"
#include "ApplyStorageModificationsTag.h"

namespace sirius::contract::storage {

RPCStorageModification::RPCStorageModification(GlobalEnvironment& environment,
                                               std::weak_ptr<RPCClient> pRPCClient,
                                               const DriveKey& driveKey)
        : m_environment(environment)
          , m_pRPCClient(std::move(pRPCClient))
          , m_driveKey(driveKey) {}

void RPCStorageModification::initiateSandboxModification(
        std::shared_ptr<AsyncQueryCallback<std::unique_ptr<SandboxModification>>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    auto sandboxModification = std::make_unique<RPCSandboxModification>(m_environment, m_pRPCClient, m_driveKey);
    auto[_, tagCallback] = createAsyncQuery<void>(
            [callback = std::move(callback), sandboxModification=std::move(sandboxModification)]
            (auto&& res) mutable {
        if (!res) {
            callback->postReply(tl::unexpected<std::error_code>(res.error()));
            return;
        }
        callback->postReply(std::move(sandboxModification));
    }, [] {}, m_environment, false, true);

    storageServer::InitSandboxRequest request;
    request.set_drive_key(m_driveKey.toString());
    auto* tag = new InitiateSandboxModificationsTag(m_environment,
                                                    std::move(request),
                                                    rpcClient->stub(),
                                                    rpcClient->completionQueue(),
                                                    std::move(tagCallback));
    rpcClient->addTag(tag);
    tag->start();
}

void RPCStorageModification::evaluateStorageHash(std::shared_ptr<AsyncQueryCallback<StorageState>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::EvaluateStorageHashRequest request;
    request.set_drive_key(m_driveKey.toString());
    auto* tag = new EvaluateStorageHashTag(m_environment,
                                           std::move(request),
                                           rpcClient->stub(),
                                           rpcClient->completionQueue(),
                                           std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}

void
RPCStorageModification::applyStorageModifications(bool success, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto rpcClient = m_pRPCClient.lock();
    if (!rpcClient) {
        callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
        return;
    }

    storageServer::ApplyStorageModificationsRequest request;
    request.set_drive_key(m_driveKey.toString());
    request.set_success(success);
    auto* tag = new ApplyStorageModificationsTag(m_environment,
                                                 std::move(request),
                                                 rpcClient->stub(),
                                                 rpcClient->completionQueue(),
                                                 std::move(callback));
    rpcClient->addTag(tag);
    tag->start();
}
}