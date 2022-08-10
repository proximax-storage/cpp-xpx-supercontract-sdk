/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <boost/beast/ssl.hpp>

#include "supercontract/Storage.h"
#include "supercontract/Messenger.h"
#include "supercontract/ExecutorEventHandler.h"
#include "supercontract/ExecutorConfig.h"
#include "VirtualMachine.h"
#include "crypto/KeyPair.h"
#include "VirtualMachineQueryHandlersKeeper.h"
#include "VirtualMachineInternetQueryHandler.h"

#include "supercontract/ThreadManager.h"

namespace sirius::contract {

class ExecutorEnvironment {

public:

    virtual ~ExecutorEnvironment() = default;

    virtual const crypto::KeyPair& keyPair() const = 0;

    virtual ThreadManager& threadManager() = 0;

    virtual Messenger& messenger() = 0;

    virtual Storage& storage() = 0;

    virtual ExecutorEventHandler& executorEventHandler() = 0;

    virtual std::weak_ptr<VirtualMachine> virtualMachine() = 0;

    virtual ExecutorConfig& executorConfig() = 0;

    virtual std::weak_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineInternetQueryHandler>> internetHandlerKeeper() = 0;

    virtual boost::asio::ssl::context& sslContext() = 0;
};

}