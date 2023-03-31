/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <tl/expected.hpp>
#include <system_error>
#include "executor.pb.h"

namespace sirius::contract::executor {

class ExecutorClientMessageSender {

public:

    virtual ~ExecutorClientMessageSender() = default;

    virtual void sendMessage(executor_server::ClientMessage&& request) = 0;
};

}