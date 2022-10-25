/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <supercontract/Identifiers.h>
#include "Message.h"

namespace sirius::contract::messenger {

class Messenger {

public:

    virtual ~Messenger() = default;

    virtual void sendMessage(const ExecutorKey& receiver, const std::string& tag, const std::string& message) = 0;

};

}