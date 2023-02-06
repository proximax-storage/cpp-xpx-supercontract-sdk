/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "messenger/Messenger.h"

namespace sirius::contract::test {

class MessengerMock : public messenger::Messenger {

public:

    std::map<ExecutorKey, std::map<std::string, std::vector<std::string>>> m_sentMessages;

    void sendMessage(const messenger::OutputMessage& message) override;
};


} // namespace sirius::contract::test