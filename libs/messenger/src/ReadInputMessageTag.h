/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCTag.h"
#include <common/SingleThread.h>
#include <common/AsyncQuery.h>
#include <messenger/Message.h>
#include <messengerServer.pb.h>

namespace sirius::contract::messenger {

class ReadInputMessageTag : public RPCTag, private SingleThread {

private:

    GlobalEnvironment& m_environment;
    std::shared_ptr<AsyncQueryCallback<InputMessage>> m_callback;

public:

    messengerServer::ServerMessage m_response;

public:

    ReadInputMessageTag(GlobalEnvironment& environment,
                        std::shared_ptr<AsyncQueryCallback<InputMessage>>&& callback);

    void process(bool ok) override;

};

}