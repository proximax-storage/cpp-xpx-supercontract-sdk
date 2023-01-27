/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "executor/DefaultExecutorBuilder.h"
#include "DefaultExecutor.h"

namespace sirius::contract {

std::shared_ptr<Executor> DefaultExecutorBuilder::build(crypto::KeyPair&& keyPair, const ExecutorConfig& config,
                                                        const std::shared_ptr<ExecutorEventHandler>& eventHandler,
                                                        std::unique_ptr<vm::VirtualMachineBuilder>&& vmBuilder,
                                                        std::unique_ptr<ServiceBuilder<storage::Storage>>&& storageBuilder,
                                                        std::unique_ptr<ServiceBuilder<blockchain::Blockchain>>&& blockchainBuilder,
                                                        std::unique_ptr<messenger::MessengerBuilder>&& messengerBuilder,
                                                        const std::string& dbgPeerName) {
    return std::make_shared<DefaultExecutor>(std::move(keyPair), config, eventHandler, std::move(vmBuilder),
                                            std::move(storageBuilder), std::move(blockchainBuilder),
                                            std::move(messengerBuilder), dbgPeerName);
}

}