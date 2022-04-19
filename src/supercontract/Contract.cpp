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
#include "ContractConfig.h"
#include "ContractContext.h"

namespace sirius::contract {

class DefaultContract : public Contract, public TaskContext {

private:

    ContractKey             m_contractKey;

    DriveKey                m_driveKey;
    std::set<ExecutorKey>   m_executors;

    ContractContext&        m_contractContext;
    ContractConfig          m_contractConfig;

    ProofOfExecution                    m_proofOfExecution;
    std::unique_ptr<BaseBatchesManager> m_batchesManager;

    std::unique_ptr<BaseContractTask>   m_task;

    std::map<uint64_t, std::map<ExecutorKey, EndBatchExecutionTransactionInfo>> m_unknownSuccessfulBatchInfos;
    std::map<uint64_t, std::map<ExecutorKey, EndBatchExecutionTransactionInfo>> m_unknownUnsuccessfulBatchInfos;

public:

    DefaultContract( const ContractKey& contractKey,
                     const AddContractRequest& addContractRequest,
                     ContractContext& contractContext,
                     const ExecutorConfig& executorConfig)
            : m_contractKey( contractKey )
            , m_driveKey( addContractRequest.m_driveKey )
            , m_executors(addContractRequest.m_executors)
            , m_contractContext(contractContext)
            , m_contractConfig(executorConfig)
            {}

    void addContractCall( const CallRequest& request ) override {
        m_batchesManager->addCall( request );
        if ( !m_task ) {
            runTask();
        }
    }

    // region storage bridge event handler

    bool onStorageSynchronized( uint64_t batchIndex ) override {
        if ( !m_task ) {
            return false;
        }

        m_batchesManager->clearOutdatedBatches( batchIndex );
        m_task->onStorageSynchronized( batchIndex );

        // TODO should we always return true?
        return true;
    }

    bool onInitiatedModifications( uint64_t batchIndex ) override {
        if ( !m_task ) {
            return false;
        }

        return m_task->onInitiatedModifications( batchIndex );
    }

    bool onAppliedSandboxStorageModifications( uint64_t batchIndex, bool success, int64_t sandboxSizeDelta, int64_t stateSizeDelta ) override {
        if ( !m_task ) {
            return false;
        }

        return m_task->onAppliedSandboxStorageModifications(batchIndex, success, sandboxSizeDelta, stateSizeDelta);
    }

    bool
    onRootHashEvaluated( uint64_t batchIndex, const Hash256& rootHash, uint64_t usedDriveSize, uint64_t metaFilesSize,
                         uint64_t fileStructureSize ) override {
        if ( !m_task ) {
            return false;
        }

        return m_task->onRootHashEvaluated( batchIndex, rootHash, usedDriveSize, metaFilesSize, fileStructureSize );
    }

    bool onAppliedStorageModifications( uint64_t batchIndex ) override {
        if ( !m_task ) {
            return false;
        }

        return m_task->onAppliedStorageModifications( batchIndex );
    }

    // endregion

    bool onSuperContractCallExecuted( const CallExecutionResult& executionResult ) override {
        if ( !m_task ) {
            return false;
        }

        return m_task->onSuperContractCallExecuted(executionResult);
    }


    bool onEndBatchExecutionOpinionReceived( const EndBatchExecutionTransactionInfo& info ) override {
        if (!m_task || !m_task->onEndBatchExecutionOpinionReceived(info)) {
            if (info.isSuccessful()) {
                m_unknownSuccessfulBatchInfos[info.m_batchIndex][info.m_executorKeys.front()] = info;
            }
            else {
                m_unknownUnsuccessfulBatchInfos[info.m_batchIndex][info.m_executorKeys.front()] = info;
            }
        }

        return true;
    }

    // region task context

    const ContractKey& contractKey() const override {
        return m_contractKey;
    }

    const std::set<ExecutorKey>& executors() const override {
        return m_executors;
    }

    const crypto::KeyPair& keyPair() const override {
        return m_contractContext.keyPair();
    }

    ThreadManager& threadManager() override {
        return m_contractContext.threadManager();
    }

    Messenger& messenger() override {
        return m_contractContext.messenger();
    }

    StorageBridge& storageBridge() override {
        return m_contractContext.storageBridge();
    }

    ExecutorEventHandler& executorEventHandler() override {
        return m_contractContext.executorEventHandler();
    }

    const ContractConfig& contractConfig() const override {
        return m_contractConfig;
    }

    void onTaskFinished() override {

    }

    std::string dbgPeerName() override {
        return m_contractContext.dbgPeerName();
    }

    // end region

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