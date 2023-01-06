/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "executor/DefaultExecutorBuilder.h"
#include "DefaultExecutor.h"

namespace sirius::contract {

std::unique_ptr<Executor>
DefaultExecutorBuilder::build(crypto::KeyPair&& keyPair,
                              const ExecutorConfig& config,
                              std::unique_ptr<ExecutorEventHandler>&& eventHandler,
                              const std::string& dbgPeerName) {
    return std::make_unique<DefaultExecutor>(std::move(keyPair),
                                             config,
                                             std::move(eventHandler),
                                             dbgPeerName);
}

}