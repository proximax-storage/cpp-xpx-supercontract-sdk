/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/SingleThread.h"
#include <storage/StorageContentManager.h>
#include <storageContentManagerServer.grpc.pb.h>
#include <grpcpp/impl/codegen/completion_queue.h>

namespace rpc = ::storage;

namespace sirius::contract::storage {

class RPCStorageContentManager
        : private SingleThread
        , public StorageContentManager {

private:

    GlobalEnvironment& m_environment;

    std::unique_ptr<rpc::StorageContentManagerServer::Stub> m_stub;

    grpc::CompletionQueue m_completionQueue;
    std::thread m_completionQueueThread;

public:

    RPCStorageContentManager(
            GlobalEnvironment& environment,
            const std::string& serverAddress);

    ~RPCStorageContentManager() override;

    void getAbsolutePath(
            const DriveKey& driveKey,
            const std::string& relativePath,
            std::shared_ptr<AsyncQueryCallback<std::string>> callback) override;

private:

    void waitForRPCResponse();
};

}