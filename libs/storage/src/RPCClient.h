/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <storage/SandboxModification.h>
#include <storageServer.grpc.pb.h>
#include "RPCTag.h"

namespace sirius::contract::storage {

class RPCClient : private SingleThread {

private:
    GlobalEnvironment& m_environment;

    std::unique_ptr<storageServer::StorageServer::Stub> m_stub;

    grpc::CompletionQueue m_completionQueue;
    std::thread m_completionQueueThread;

    // This set does NOT own the tags
    std::set<RPCTag*> m_activeTags;

public:

    RPCClient(
            GlobalEnvironment& environment,
            const std::string& serverAddress);

    ~RPCClient();

public:

    storageServer::StorageServer::Stub& stub();

    grpc::CompletionQueue& completionQueue();

    void addTag(RPCTag* tag);

private:
    void waitForRPCResponse();

};

}