/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <messenger/RPCMessengerSession.h>

namespace sirius::contract::messenger {

void RPCMessengerSession::read(std::shared_ptr<AsyncQueryCallback<InputMessage>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(!m_readCallback, m_environment.logger())

    if (!m_receivedMessages.empty()) {
        callback->postReply(std::move(m_receivedMessages.front()));
        m_receivedMessages.pop();
    } else {
        m_readCallback = std::move(callback);
    }
}

void RPCMessengerSession::write(const OutputMessage& message) {
    ASSERT(isSingleThread(), m_environment.logger())

    if (!m_writeQuery) {

    }
    else {
        m_outgoingMessages.push(message);
    }
}

void RPCMessengerSession::onRead(expected<InputMessage>&& message) {

    ASSERT(isSingleThread(), m_environment.logger())

    if (message) {
        // TODO read one more message
    }
    if (m_readCallback) {
        m_readCallback->postReply(std::move(message));
        m_readCallback.reset();
    } else {
        m_receivedMessages.emplace(std::move(message));
    }
}

}