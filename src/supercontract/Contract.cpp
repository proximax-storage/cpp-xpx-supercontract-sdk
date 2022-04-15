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
#include "ProofOfExecution.h"

namespace sirius::contract {

class DefaultContract : public Contract, public TaskContext {

private:
    ContractKey           m_contractKey;
    DriveKey              m_driveKey;
    std::set<ExecutorKey> m_executors;

    ExecutorEventHandler&               m_eventHandler;
    Messenger&                          m_messenger;
    StorageBridge&                      m_storageBridge;
    VirtualMachine&                     m_virtualMachine;

    ProofOfExecution                    m_proofOfExecution;
    std::unique_ptr<BaseBatchesManager> m_batchesManager;

    std::unique_ptr<BaseContractTask>   m_task;

    std::map<Hash256, std::map<ExecutorKey, EndBatchExecutionTransactionInfo>> m_unknownSuccessfulBatchInfos;
    std::map<Hash256, std::map<ExecutorKey, EndBatchExecutionTransactionInfo>> m_unknownUnsuccessfulBatchInfos;

public:

    DefaultContract( const ContractKey& contractKey,
                     const AddContractRequest& addContractRequest,
                     ExecutorEventHandler& eventHandler,
                     Messenger& messenger,
                     StorageBridge& storageBridge,
                     VirtualMachine& virtualMachine )
            : m_contractKey( contractKey ), m_driveKey( addContractRequest.m_driveKey ), m_executors(addContractRequest.m_executors),
              m_eventHandler( eventHandler ), m_messenger( messenger ), m_storageBridge( storageBridge ),
              m_virtualMachine( virtualMachine ) {

    }

    void addContractCall( const CallRequest& request ) override {
        m_batchesManager->addCall( request );
        if ( !m_task ) {
            runTask();
        }
    }

    bool onStorageSynchronized() override {
        if ( !m_task ) {
            return false;
        }

        return m_task->onStorageSynchronized();
    }

    bool onStorageModificationsApplied() override {
        if ( !m_task ) {
            return false;
        }

        return m_task->onStorageModificationsApplied();
    }

    bool onSuperContractCallExecuted( const CallExecutionResult& executionResult ) override {
        if ( !m_task ) {
            return false;
        }

        return m_task->onSuperContractCallExecuted(executionResult);
    }


    bool onEndBatchExecutionOpinionReceived( const EndBatchExecutionTransactionInfo& info ) override {
        if (!m_task || !m_task->onEndBatchExecutionOpinionReceived(info)) {
            if (info.isSuccessful()) {
                m_unknownSuccessfulBatchInfos[info.m_batchId][info.m_executorKeys.front()] = info;
            }
            else {
                m_unknownUnsuccessfulBatchInfos[info.m_batchId][info.m_executorKeys.front()] = info;
            }
        }

        return true;
    }

private:

    void runTask() {
        if ( m_batchesManager->hasFormedBatch()) {
        }

        if ( m_task ) {
            m_task->run();
        }
    }

    void runBatchExecutionTask() {
        m_task = createBatchExecutionTask( m_batchesManager->popFormedBatch(), *this, m_virtualMachine );
        m_task->run();
    }

    void runInitializeContractTask() {
    }

};

std::unique_ptr<Contract>
createDefaultContract( const ContractKey& contractKey, const AddContractRequest& addContractRequest,
                       ExecutorEventHandler& eventHandler, Messenger& messenger, StorageBridge& storageBridge, VirtualMachine& virtualMachine ) {
    return std::make_unique<DefaultContract>( contractKey,
                                              addContractRequest,
                                              eventHandler,
                                              messenger,
                                              storageBridge,
                                              virtualMachine);
}

}