/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "ContractEnvironment.h"
#include "ExecutorEnvironment.h"
#include "ContractMessageEventHandler.h"
#include "ContractBlockchainEventHandler.h"

namespace sirius::contract {

class BaseContractTask
        : protected SingleThread
        , public ContractMessageEventHandler
        , public ContractBlockchainEventHandler {

protected:

    ExecutorEnvironment& m_executorEnvironment;
    ContractEnvironment& m_contractEnvironment;

    ModificationId storageModificationId(uint64_t batchId);

public:

    explicit BaseContractTask(
            ExecutorEnvironment& executorEnvironment,
            ContractEnvironment& contractEnvironment);

    virtual void run() = 0;

    virtual void terminate() = 0;
};

}