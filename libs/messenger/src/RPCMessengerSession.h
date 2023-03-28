/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "messengerServer.grpc.pb.h"
#include <messenger/Message.h>
#include <queue>
#include <common/AsyncQuery.h>
#include <common/GlobalEnvironment.h>
#include <common/SingleThread.h>

namespace sirius::contract::messenger {

class RPCMessengerSession : private SingleThread {

private:

    GlobalEnvironment& m_environment;

    messengerServer::MessengerServer::Stub& m_stub;

    grpc::CompletionQueue m_completionQueue;

    grpc::ClientContext m_context;

    std::unique_ptr<grpc::ClientAsyncReaderWriter<messengerServer::ClientMessage, messengerServer::ServerMessage>> m_stream;

    std::thread m_completionQueueThread;

public:

    RPCMessengerSession(GlobalEnvironment& environment,
                        messengerServer::MessengerServer::Stub& stub);

    virtual ~RPCMessengerSession();

public:

    void initiate(std::shared_ptr<AsyncQueryCallback<void>> callback);

    void read(std::shared_ptr<AsyncQueryCallback<InputMessage>> callback);

    void write(const OutputMessage& message, std::shared_ptr<AsyncQueryCallback<void>> callback);

    void subscribe(const std::string& tag, std::shared_ptr<AsyncQueryCallback<void>> callback);

private:

    void waitForRPCResponse();

};

}