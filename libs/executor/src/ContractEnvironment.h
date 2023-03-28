/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <messenger/Messenger.h>
#include "executor/ExecutorEventHandler.h"
#include "crypto/KeyPair.h"

#include <common/ThreadManager.h>
#include "TaskRequests.h"
#include "ContractConfig.h"
#include "Batch.h"
#include "ProofOfExecution.h"
#include "BatchesManager.h"

namespace sirius::contract {

class ContractEnvironment {

public:

    virtual ~ContractEnvironment() = default;

    virtual const ContractKey& contractKey() const = 0;

    virtual const DriveKey& driveKey() const = 0;

    virtual const std::map<ExecutorKey, ExecutorInfo>& executors() const = 0;

    virtual uint64_t automaticExecutionsSCLimit() const = 0;

    virtual uint64_t automaticExecutionsSMLimit() const = 0;

    virtual const ContractConfig& contractConfig() const = 0;

    virtual void addSynchronizationTask() = 0;

    virtual void notifyHasNextBatch() = 0;

    virtual BaseBatchesManager& batchesManager() = 0;

    virtual ProofOfExecution& proofOfExecution() = 0;
};

}