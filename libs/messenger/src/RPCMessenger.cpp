/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <messenger/RPCMessenger.h>

namespace sirius::contract::messenger {

RPCMessenger::RPCMessenger(GlobalEnvironment& environment,
                           std::weak_ptr<MessageSubscriber> subscriber,
                           std::set<std::string> subscribedTags)
        : m_environment(environment)
          , m_subscriber(std::move(subscriber))
          , m_subscribedTags(std::move(subscribedTags)) {}

void RPCMessenger::sendMessage(const ExecutorKey& receiver, const std::string& tag, const std::string& message) {

}

void RPCMessenger::write() {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(!m_writeQuery, m_environment.logger())

    if (!isSessionActive()) {
        return;
    }

    if (!m_queuedTags.empty()) {
        auto tag = std::move(m_queuedTags.front());
        m_queuedTags.pop();
        auto[query, callback] = createAsyncQuery<void>([this](auto&& res) {
            onWritten(res);
        }, [] {}, m_environment, true, true);
        m_writeQuery = std::move(query);
        m_rpcSession->subscribe(tag, callback);
    } else if (!m_queuedMessages.empty()) {
        auto message = std::move(m_queuedMessages.front());
        m_queuedMessages.pop();
        auto[query, callback] = createAsyncQuery<void>([this](auto&& res) {
            onWritten(res);
        }, [] {}, m_environment, true, true);
        m_writeQuery = std::move(query);
        m_rpcSession->write(message, callback);
    }
}

void RPCMessenger::read() {
    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(isSessionActive(), m_environment.logger())

    ASSERT(!m_readQuery, m_environment.logger())

    auto [query, callback] = createAsyncQuery<InputMessage>([this] (auto&& res) {
        onRead(res);
    })
}

void RPCMessenger::onSessionInitiated(expected<void>&& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_sessionInitiateQuery, m_environment.logger())

    ASSERT(m_rpcSession, m_environment.logger())

    m_sessionInitiateQuery.reset();

    if (!res) {
        restartSession();
    }
    else {
        write();
    }
}

void RPCMessenger::onWritten(const expected<void>& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_writeQuery, m_environment.logger())

    ASSERT(isSessionActive(), m_environment.logger())

    if (!res) {
        restartSession();
        return;
    }

    m_writeQuery.reset();
    write();
}

void RPCMessenger::onRead(const expected<InputMessage>& res) {
    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_readQuery, m_environment.logger())

    ASSERT(isSessionActive(), m_environment.logger())

    if (!res) {
        restartSession();
        return;
    }

    if (m_subscribedTags.contains(res->m_tag)) {
        auto subscriber = m_subscriber.lock();
        if (subscriber) {
            subscriber->onMessageReceived(*res);
        }
    }
    else {
        m_environment.logger().warn("Received Message With Unknown Tag: {}", res->m_tag);
    }

    m_readQuery.reset();
    read();
}

void RPCMessenger::stopSession() {

    ASSERT(isSingleThread(), m_environment.logger())

    m_sessionInitiateQuery.reset();
    m_writeQuery.reset();
    m_readQuery.reset();
    m_rpcSession.reset();
    m_restartTimer.cancel();
}

void RPCMessenger::startSession() {

//    auto [query, callback] = createAsyncQuery<>()
}

void RPCMessenger::restartSession() {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(!m_restartTimer, m_environment.logger())

    stopSession();
    m_restartTimer = Timer(m_environment.threadManager().context(), 15000, [this] {
        startSession();
    });
}

bool RPCMessenger::isSessionActive() {
    return m_rpcSession && !m_sessionInitiateQuery;
}

}