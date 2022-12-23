/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <messenger/RPCMessenger.h>
#include <gtest/gtest.h>
#include "TestUtils.h"

namespace sirius::contract::messenger::test {

class MessageSubscriberImpl : public MessageSubscriber {
public:

    std::vector<InputMessage> m_expected;
    std::set<std::string> m_tags;
    std::promise<void>& m_promise;

    MessageSubscriberImpl(std::promise<void>& promise) : m_promise(promise) {}

private:

    int m_next = 0;

    void onMessageReceived(const InputMessage& message) override {
        ASSERT_TRUE(m_next < m_expected.size());

        const auto& expectedMessage = m_expected[m_next];

        ASSERT_EQ(expectedMessage.m_tag, message.m_tag);
        ASSERT_EQ(expectedMessage.m_content, message.m_content);

        m_next++;
        if (m_next == m_expected.size()) {
            m_promise.set_value();
        }
    }

    std::set<std::string> subscriptions() override {
        return m_tags;
    }

};

TEST(Messenger, Echo) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    std::promise<void> p;
    auto barrier = p.get_future();

    std::shared_ptr<Messenger> messenger;

    MessageSubscriberImpl messageSubscriber(p);

    threadManager.execute([&] {
        std::string address = "127.0.0.1:5052";

        int r = 100;

        for (int i = 0; i < r; i++) {
            messageSubscriber.m_tags.insert("tag_" + std::to_string(i));
        }

        messenger = std::make_shared<RPCMessenger>(environment, address, messageSubscriber);

        for (int i = 0; i < 2 * r; i++) {
            std::string tag = "tag_" + std::to_string(i / 2);
            std::string content = "message_" + std::to_string(i);
            messageSubscriber.m_expected.push_back({tag, content});
        }

        for (const auto& message: messageSubscriber.m_expected) {
            messenger->sendMessage(OutputMessage{Key(), message.m_tag, message.m_content});
        }
    });

    barrier.get();

    threadManager.execute([&] {
        messenger.reset();
    });

    threadManager.stop();
}
}