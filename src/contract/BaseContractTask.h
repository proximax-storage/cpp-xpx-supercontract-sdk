/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/Identifiers.h"
#include "supercontract/Requests.h"
#include "supercontract/Transactions.h"
#include "supercontract/StorageQueries.h"
#include "ContractStorageEventHandler.h"
#include "ContractMessageEventHandler.h"
#include "ContractBlockchainEventHandler.h"
#include "TaskRequests.h"
#include "BatchesManager.h"
#include "ContractEnvironment.h"
#include "ExecutorEnvironment.h"
#include "../virtualMachine/include/virtualMachine/VirtualMachine.h"
#include "DebugInfo.h"
#include "log.h"

namespace sirius::contract {

class BaseContractTask
        : public ContractStorageEventHandler,
          public ContractMessageEventHandler,
          public ContractBlockchainEventHandler {
protected:

    ExecutorEnvironment& m_executorEnvironment;
    ContractEnvironment& m_contractEnvironment;
    const DebugInfo m_dbgInfo;

public:

    explicit BaseContractTask(
            ExecutorEnvironment& executorEnvironment,
            ContractEnvironment& contractEnvironment,
            const DebugInfo& debugInfo )
            : m_executorEnvironment( executorEnvironment )
            , m_contractEnvironment( contractEnvironment )
            , m_dbgInfo( debugInfo )
    {}

    virtual void run() = 0;

    virtual void terminate() = 0;

};

std::unique_ptr<BaseContractTask> createInitContractTask( AddContractRequest&& request,
                                                          ContractEnvironment& contractEnvironment,
                                                          ExecutorEnvironment& executorEnvironment,
                                                          const DebugInfo& );

std::unique_ptr<BaseContractTask> createBatchExecutionTask( Batch&& batch,
                                                            ContractEnvironment& contractEnvironment,
                                                            ExecutorEnvironment& executorEnvironment,
                                                            std::map<ExecutorKey, EndBatchExecutionOpinion>&& otherSuccessfulExecutorEndBatchInfos,
                                                            std::map<ExecutorKey, EndBatchExecutionOpinion>&& otherUnsuccessfulExecutorEndBatchInfos,
                                                            std::optional<PublishedEndBatchExecutionTransactionInfo>&& publishedEndBatchInfo,
                                                            const DebugInfo& );

std::unique_ptr<BaseContractTask> createSynchronizationTask( SynchronizationRequest&& storageState,
                                                             ContractEnvironment& contractEnvironment,
                                                             ExecutorEnvironment& executorEnvironment,
                                                             const DebugInfo& );

std::unique_ptr<BaseContractTask> createRemoveContractTask( RemoveRequest&& request, ContractEnvironment&, ExecutorEnvironment&, const DebugInfo& );

}