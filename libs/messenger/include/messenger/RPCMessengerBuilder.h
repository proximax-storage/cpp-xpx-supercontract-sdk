/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "MessengerBuilder.h"
#include "Messenger.h"

namespace sirius::contract::messenger {

class RPCMessengerBuilder: public MessengerBuilder {

public:

    RPCMessengerBuilder(const std::string& address);

    std::shared_ptr<Messenger> build(GlobalEnvironment& environment) override;

};

}