/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <string>
#include "Message.h"

namespace sirius::contract::messenger {

class MessageSubscriber {

public:

    virtual ~MessageSubscriber() = default;

    virtual void onMessageReceived(const InputMessage&) = 0;

    virtual std::set<std::string> subscriptions() = 0;

};

}