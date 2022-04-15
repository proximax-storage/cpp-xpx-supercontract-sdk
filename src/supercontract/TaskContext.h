/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "contract/StorageBridge.h"
#include "contract/Messenger.h"
#include "contract/ExecutorEventHandler.h"
#include "crypto/KeyPair.h"

#include "ThreadManager.h"

namespace sirius::contract {

class TaskContext {

public:

    virtual ~TaskContext() = default;

    virtual const ContractKey& contractKey() = 0;

    virtual const std::vector<ExecutorKey>& executors() = 0;

    virtual const crypto::KeyPair& keyPair() = 0;

    virtual ThreadManager& threadManager() = 0;

    virtual Messenger& messenger() = 0;

    virtual StorageBridge& storageBridge() = 0;

    virtual ExecutorEventHandler& executorEventHandler() = 0;

    virtual void onTaskFinished() = 0;

    virtual std::string dbgPeerName() = 0;

};

}