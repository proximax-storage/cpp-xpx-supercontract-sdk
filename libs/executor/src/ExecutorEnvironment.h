/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <boost/beast/ssl.hpp>

#include <storage/Storage.h>
#include <messenger/Messenger.h>
#include "executor/ExecutorEventHandler.h"
#include "executor/ExecutorConfig.h"
#include <common/GlobalEnvironment.h>
#include <virtualMachine/VirtualMachine.h>
#include "crypto/KeyPair.h"
#include <virtualMachine/VirtualMachineInternetQueryHandler.h>
#include <virtualMachine/VirtualMachine.h>
#include <blockchain/Blockchain.h>

#include <common/ThreadManager.h>

namespace sirius::contract {

class ExecutorEnvironment: public GlobalEnvironment {

public:

    ExecutorEnvironment(std::shared_ptr<logging::Logger> logger)
    : GlobalEnvironment(std::move(logger)) {}

    ExecutorEnvironment(std::shared_ptr<logging::Logger> logger,
                        std::shared_ptr<ThreadManager> threadManager)
    : GlobalEnvironment(std::move(logger), std::move(threadManager)) {}

    ~ExecutorEnvironment() override = default;

    virtual const crypto::KeyPair& keyPair() const = 0;

    virtual std::weak_ptr<messenger::Messenger> messenger() = 0;

    virtual std::weak_ptr<storage::Storage> storage() = 0;

    virtual std::weak_ptr<blockchain::Blockchain> blockchain() = 0;

    virtual ExecutorEventHandler& executorEventHandler() = 0;

    virtual std::weak_ptr<vm::VirtualMachine> virtualMachine() = 0;

	virtual ExecutorConfig& executorConfig() = 0;

    virtual boost::asio::ssl::context& sslContext() = 0;
};

}