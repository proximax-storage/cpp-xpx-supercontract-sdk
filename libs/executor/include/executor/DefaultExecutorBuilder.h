/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <crypto/KeyPair.h>
#include "Executor.h"
#include "ExecutorEventHandler.h"
#include <virtualMachine/VirtualMachineBuilder.h>
#include <messenger/MessengerBuilder.h>
#include <common/ThreadManager.h>
#include <storage/Storage.h>
#include <blockchain/Blockchain.h>

namespace sirius::contract {

class DefaultExecutorBuilder {

public:

    DefaultExecutorBuilder() = default;

    std::shared_ptr<Executor> build(crypto::KeyPair&& keyPair,
                                    const ExecutorConfig& config,
                                    const std::shared_ptr<ExecutorEventHandler>& eventHandler,
                                    std::unique_ptr<vm::VirtualMachineBuilder>&& vmBuilder,
                                    std::unique_ptr<ServiceBuilder<storage::Storage>>&& storageBuilder,
                                    std::unique_ptr<ServiceBuilder<blockchain::Blockchain>>&& blockchainBuilder,
                                    std::unique_ptr<messenger::MessengerBuilder>&& messengerBuilder,
                                    std::shared_ptr<logging::Logger> logger);

};

}