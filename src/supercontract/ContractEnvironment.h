/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "contract/Storage.h"
#include "contract/Messenger.h"
#include "contract/ExecutorEventHandler.h"
#include "crypto/KeyPair.h"

#include "ThreadManager.h"
#include "TaskRequests.h"
#include "ContractConfig.h"

namespace sirius::contract {

class ContractEnvironment {

public:

    virtual ~ContractEnvironment() = default;

    virtual const ContractKey& contractKey() const = 0;

    virtual const DriveKey& driveKey() const = 0;

    virtual const std::set<ExecutorKey>& executors() const = 0;

    virtual const ContractConfig& contractConfig() const = 0;

    virtual void finishTask() = 0;

    virtual void addSynchronizationTask( const SynchronizationRequest& ) = 0;

};

}