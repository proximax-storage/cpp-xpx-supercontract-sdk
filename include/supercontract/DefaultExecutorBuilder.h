/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <crypto/KeyPair.h>
#include "Executor.h"
#include "ExecutorEventHandler.h"
#include "ThreadManager.h"
#include "Messenger.h"
#include "Storage.h"
#include "StorageObserver.h"

namespace sirius::contract {

class DefaultExecutorBuilder {

public:

    DefaultExecutorBuilder() = default;

    std::unique_ptr<Executor> build(const crypto::KeyPair& keyPair,
                                    std::shared_ptr<ThreadManager> pThreadManager,
                                    const ExecutorConfig& config,
                                    std::unique_ptr<ExecutorEventHandler>&& eventHandler,
                                    Messenger& messenger,
                                    storage::Storage& storage,
                                    const StorageObserver& storageObserver,
                                    const std::string& dbgPeerName = "executor");

};

}