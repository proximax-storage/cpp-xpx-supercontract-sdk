/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <cereal/archives/portable_binary.hpp>

namespace sirius::contract {

class MessengerEventHandler {
public:

    virtual ~MessengerEventHandler() = default;

    virtual void onMessageReceived(const std::string& tag, const std::string& msg) = 0;

};

}