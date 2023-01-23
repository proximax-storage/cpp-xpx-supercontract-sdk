/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <messenger/Messenger.h>
#include <messenger/MessageSubscriber.h>
#include "RPCMessengerSession.h"
#include <supercontract/GlobalEnvironment.h>
#include <supercontract/AsyncQuery.h>
#include <memory>
#include <queue>

namespace sirius::contract::messenger {

class RPCMessenger : public Messenger, private SingleThread {

private:

    GlobalEnvironment& m_environment;

    std::unique_ptr<messengerServer::MessengerServer::Stub> m_stub;

    MessageSubscriber& m_subscriber;
    std::set<std::string> m_subscribedTags;

    std::queue<std::string> m_queuedTags;
    std::queue<OutputMessage> m_queuedMessages;

    std::shared_ptr<AsyncQuery> m_sessionInitiateQuery;
    std::shared_ptr<AsyncQuery> m_writeQuery;
    std::shared_ptr<AsyncQuery> m_readQuery;

    std::unique_ptr<RPCMessengerSession> m_rpcSession;

    Timer m_restartTimer;

public:

    RPCMessenger(GlobalEnvironment& environment,
                 const std::string& address,
                 MessageSubscriber& subscriber);

    ~RPCMessenger() override;

public:

    void sendMessage(const OutputMessage& message) override;

private:

    void onSessionInitiated(expected<void>&& res);

    void write();

    void read();

    void stopSession();

    void startSession();

    void restartSession();

    void onWritten(expected<void>&& res);

    void onRead(expected<InputMessage>&& res);

    bool isSessionActive();

};

}