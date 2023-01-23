/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "RPCMessenger.h"
#include <grpcpp/security/credentials.h>
#include <grpcpp/create_channel.h>
#include "ReadInputMessageTag.h"

namespace sirius::contract::messenger {

RPCMessenger::RPCMessenger(GlobalEnvironment& environment,
                           const std::string& address,
                           MessageSubscriber& subscriber)
        : m_environment(environment)
        , m_stub(messengerServer::MessengerServer::NewStub(grpc::CreateChannel(
                address, grpc::InsecureChannelCredentials())))
        , m_subscriber(subscriber)
        , m_subscribedTags(m_subscriber.subscriptions()) {
    startSession();
}

RPCMessenger::~RPCMessenger() {

    ASSERT(isSingleThread(), m_environment.logger())

    stopSession();
}

void RPCMessenger::sendMessage(const OutputMessage& message) {

    ASSERT(isSingleThread(), m_environment.logger())

    m_queuedMessages.push(message);

    if (!m_writeQuery) {
        write();
    }
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
            onWritten(std::forward<expected<void>>(res));
        }, [] {}, m_environment, true, true);
        m_writeQuery = std::move(query);
        m_rpcSession->subscribe(tag, std::move(callback));
    } else if (!m_queuedMessages.empty()) {
        auto message = std::move(m_queuedMessages.front());
        m_queuedMessages.pop();
        auto[query, callback] = createAsyncQuery<void>([this](auto&& res) {
            onWritten(std::forward<expected<void>>(res));
        }, [] {}, m_environment, true, true);
        m_writeQuery = std::move(query);
        m_rpcSession->write(message, std::move(callback));
    }
}

void RPCMessenger::read() {
    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(isSessionActive(), m_environment.logger())

    ASSERT(!m_readQuery, m_environment.logger())

    auto[query, callback] = createAsyncQuery<InputMessage>([this](auto&& res) {
        onRead(std::forward<decltype(res)>(res));
    }, [] {}, m_environment, true, true);

    m_readQuery = std::move(query);

    m_rpcSession->read(callback);
}

void RPCMessenger::onSessionInitiated(expected<void>&& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_sessionInitiateQuery, m_environment.logger())

    ASSERT(m_rpcSession, m_environment.logger())

    m_sessionInitiateQuery.reset();

    if (!res) {
        restartSession();
        return;
    }

    for (const auto& tag: m_subscribedTags) {
        m_queuedTags.push(tag);
    }

    read();
    write();
}

void RPCMessenger::onWritten(expected<void>&& res) {

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

void RPCMessenger::onRead(expected<InputMessage>&& res) {
    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_readQuery, m_environment.logger())

    ASSERT(isSessionActive(), m_environment.logger())

    if (!res) {
        restartSession();
        return;
    }

    if (m_subscribedTags.contains(res->m_tag)) {
        m_subscriber.onMessageReceived(*res);
    } else {
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

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(!m_rpcSession, m_environment.logger())

    ASSERT(!m_sessionInitiateQuery, m_environment.logger())

    auto[query, callback] = createAsyncQuery<void>([this](auto&& res) {
        onSessionInitiated(std::forward<expected<void>>(res));
    }, [] {}, m_environment, true, true);

    m_sessionInitiateQuery = std::move(query);

    m_rpcSession = std::make_unique<RPCMessengerSession>(m_environment, *m_stub);
    m_rpcSession->initiate(std::move(callback));
}

void RPCMessenger::restartSession() {

    ASSERT(isSingleThread(), m_environment.logger())

    stopSession();
    m_restartTimer = Timer(m_environment.threadManager().context(), 15000, [this] {
        startSession();
    });
}

bool RPCMessenger::isSessionActive() {
    return m_rpcSession && !m_sessionInitiateQuery;
}

}