/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "types.h"
#include "contract/Requests.h"
#include "contract/Transactions.h"
#include "contract/StorageQueries.h"
#include "eventHandlers/ContractVirtualMachineEventHandler.h"
#include "eventHandlers/ContractStorageEventHandler.h"
#include "eventHandlers/ContractMessageEventHandler.h"
#include "eventHandlers/ContractBlockchainEventHandler.h"
#include "TaskRequests.h"
#include "BatchesManager.h"
#include "ContractEnvironment.h"
#include "ExecutorEnvironment.h"
#include "supercontract/eventHandlers/VirtualMachineEventHandler.h"
#include "VirtualMachine.h"
#include "log.h"

namespace sirius::contract {

class BaseContractTask
        : public ContractStorageEventHandler,
          public ContractVirtualMachineEventHandler,
          public ContractMessageEventHandler,
          public ContractBlockchainEventHandler {
protected:

    ExecutorEnvironment& m_executorEnvironment;
    ContractEnvironment& m_contractEnvironment;

public:

    explicit BaseContractTask(
            ExecutorEnvironment& executorEnvironment,
            ContractEnvironment& contractEnvironment )
            : m_executorEnvironment( executorEnvironment )
            , m_contractEnvironment( contractEnvironment )
    {}

    virtual void run() = 0;

    virtual void terminate() = 0;

    std::string dbgPeerName() {
        return m_executorEnvironment.dbgPeerName();
    }

};

std::unique_ptr<BaseContractTask> createBatchExecutionTask( Batch&& batch,
                                                            ContractEnvironment& taskContext,
                                                            VirtualMachine& virtualMachine,
                                                            std::map<ExecutorKey, EndBatchExecutionOpinion>&& otherSuccessfulExecutorEndBatchInfos,
                                                            std::map<ExecutorKey, EndBatchExecutionOpinion>&& otherUnsuccessfulExecutorEndBatchInfos,
                                                            std::optional<PublishedEndBatchExecutionTransactionInfo>&& publishedEndBatchInfo );

std::unique_ptr<BaseContractTask> createSynchronizationTask( SynchronizationRequest&&, ContractEnvironment& );

std::unique_ptr<BaseContractTask> createRemoveContractTask( RemoveRequest&& request, ContractEnvironment& );

}