/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "supercontract/DefaultExecutorBuilder.h"
#include "DefaultExecutor.h"

namespace sirius::contract {

std::unique_ptr<Executor>
DefaultExecutorBuilder::build(const crypto::KeyPair& keyPair,
                              std::shared_ptr<ThreadManager> pThreadManager,
                              const ExecutorConfig& config,
                              std::unique_ptr<ExecutorEventHandler>&& eventHandler,
                              const std::string& dbgPeerName) {
    return std::make_unique<DefaultExecutor>(keyPair,
                                             pThreadManager,
                                             config,
                                             std::move(eventHandler),
                                             dbgPeerName);
}

}