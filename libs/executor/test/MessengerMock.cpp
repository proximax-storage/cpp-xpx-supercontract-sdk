/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "MessengerMock.h"

namespace sirius::contract::test {

void MessengerMock::sendMessage(const sirius::contract::messenger::OutputMessage& message) {
    m_sentMessages[message.m_receiver][message.m_tag].push_back(message.m_content);
}

}