/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <messenger/Message.h>
#include <queue>
#include <supercontract/AsyncQuery.h>
#include <supercontract/GlobalEnvironment.h>
#include <supercontract/SingleThread.h>

namespace sirius::contract::messenger {

class RPCMessengerSession : private SingleThread {

private:

    GlobalEnvironment& m_environment;

public:

    void read(std::shared_ptr<AsyncQueryCallback<InputMessage>> callback);

    void write(const OutputMessage& message, std::shared_ptr<AsyncQueryCallback<void>> callback);

    void subscribe(const std::string& tag, std::shared_ptr<AsyncQueryCallback<void>> callback);

private:

    void onRead(expected<InputMessage>&& message);

};

}