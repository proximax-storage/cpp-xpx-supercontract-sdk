/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "RPCStorageContentManager.h"
#include "RPCTag.h"

namespace sirius::contract::storage {

RPCStorageContentManager::RPCStorageContentManager(GlobalEnvironment& environment,
                                                   const std::string& serverAddress)
        : m_environment(environment)
        , m_stub(std::move(rpc::StorageContentManagerServer::NewStub(grpc::CreateChannel(
                serverAddress, grpc::InsecureChannelCredentials()))))
        , m_completionQueueThread([this] {
            waitForRPCResponse();
        }) {}

RPCStorageContentManager::~RPCStorageContentManager() {

    ASSERT(isSingleThread(), m_environment.logger())

    m_completionQueue.Shutdown();

    if (m_completionQueueThread.joinable()) {
        m_completionQueueThread.join();
    }
}

void RPCStorageContentManager::waitForRPCResponse() {
    ASSERT(!isSingleThread(), m_environment.logger())

    void* pTag;
    bool ok;
    while (m_completionQueue.Next(&pTag, &ok)) {
        auto* pQuery = static_cast<RPCTag*>(pTag);
        pQuery->process(ok);
        delete pQuery;
    }
}

void RPCStorageContentManager::getAbsolutePath(const DriveKey& driveKey,
                                               const std::string& relativePath,
                                               std::shared_ptr<AsyncQueryCallback<std::optional<std::string>>> callback) {
    ASSERT(isSingleThread(), m_environment.logger());

    rpc::AbsolutePathRequest request;
}

}