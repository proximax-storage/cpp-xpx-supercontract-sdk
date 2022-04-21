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
#include "eventHandlers/ContractStorageBridgeEventHandler.h"
#include "eventHandlers/ContractMessageEventHandler.h"
#include "eventHandlers/ContractBlockchainEventHandler.h"
#include "BatchesManager.h"
#include "TaskContext.h"
#include "supercontract/eventHandlers/VirtualMachineEventHandler.h"
#include "VirtualMachine.h"
#include "log.h"

namespace sirius::contract {

class BaseContractTask
        : public ContractStorageBridgeEventHandler,
          public ContractVirtualMachineEventHandler,
          public ContractMessageEventHandler,
          public ContractBlockchainEventHandler {
protected:

    TaskContext& m_taskContext;

public:

    explicit BaseContractTask( TaskContext& taskContext )
    : m_taskContext( taskContext )
    {}

    virtual void run() = 0;

    virtual void terminate() = 0;

    std::string dbgPeerName() {
        return m_taskContext.dbgPeerName();
    }

};

std::unique_ptr<BaseContractTask> createBatchExecutionTask( Batch&& batch,
                                                            TaskContext& taskContext,
                                                            VirtualMachine& virtualMachine,
                                                            std::map<ExecutorKey, EndBatchExecutionOpinion>&& otherSuccessfulExecutorEndBatchInfos,
                                                            std::map<ExecutorKey, EndBatchExecutionOpinion>&& otherUnsuccessfulExecutorEndBatchInfos,
                                                            std::optional<PublishedEndBatchExecutionTransactionInfo>&& publishedEndBatchInfo );

std::unique_ptr<BaseContractTask> createSynchronizationTask( const Hash256&, TaskContext& );

}