/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "Contract.h"
#include "BatchesManager.h"
#include "TaskContext.h"
#include "BaseContractTask.h"
#include "ThreadManager.h"

namespace sirius::contract {

class DefaultContract : public Contract, TaskContext {

private:
    ContractKey m_contractKey;
    DriveKey m_driveKey;
    ExecutorEventHandler& m_eventHandler;
    Messenger& m_messenger;
    StorageBridge& m_storageBridge;
    std::unique_ptr<BaseBatchesManager> m_batchesManager;
    std::unique_ptr<BaseContractTask> m_task;

public:

    DefaultContract( const ContractKey& contractKey,
                     const AddContractRequest& addContractRequest,
                     ExecutorEventHandler& eventHandler,
                     Messenger& messenger,
                     StorageBridge& storageBridge )
            : m_contractKey( contractKey ), m_driveKey( addContractRequest.m_driveKey ),
              m_eventHandler( eventHandler ), m_messenger( messenger ), m_storageBridge( storageBridge ) {

    }

    void addContractCall( const CallRequest& request ) override {
        m_batchesManager->addCall(request);
        if (!m_task) {
            runTask();
        }
    }


    void onCallExecuted( const CallExecutionResult& executionResult ) override {
        if (m_task) {
            m_task->onCallExecuted(executionResult);
        }
    }

private:

    void runTask() {
        if (m_batchesManager->hasFormedBatch()) {
            m_task = createBatchExecutionTask(m_batchesManager->popFormedBatch(), *this);
        }

        if (m_task) {
            m_task->run();
        }
    }

};

std::unique_ptr<Contract>
createDefaultContract( const ContractKey& contractKey, const AddContractRequest& addContractRequest,
                       ExecutorEventHandler& eventHandler, Messenger& messenger, StorageBridge& storageBridge ) {
    return std::make_unique<DefaultContract>( contractKey,
                                              addContractRequest,
                                              eventHandler,
                                              messenger,
                                              storageBridge );
}

}