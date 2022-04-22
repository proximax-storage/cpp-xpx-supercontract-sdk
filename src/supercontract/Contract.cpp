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
#include "TaskRequests.h"

namespace sirius::contract {

class DefaultContract : public Contract, public TaskContext {

private:

    ContractKey             m_contractKey;

    DriveKey                m_driveKey;
    std::set<ExecutorKey>   m_executors;

    ContractContext&                            m_contractContext;
    ContractConfig                              m_contractConfig;

    ProofOfExecution                            m_proofOfExecution;

    // Requests
    std::unique_ptr<BaseBatchesManager>         m_batchesManager;
    std::optional<SynchronizationRequest>       m_synchronizationRequest;
    std::optional<RemoveRequest>                m_contractRemoveRequest;

    std::unique_ptr<BaseContractTask>   m_task;

    std::map<uint64_t, std::map<ExecutorKey, EndBatchExecutionOpinion>> m_unknownSuccessfulBatchOpinions;
    std::map<uint64_t, std::map<ExecutorKey, EndBatchExecutionOpinion>> m_unknownUnsuccessfulBatchOpinions;
    std::map<uint64_t, PublishedEndBatchExecutionTransactionInfo> m_unknownPublishedEndBatchTransactions;

public:

    DefaultContract( const ContractKey& contractKey,
                     AddContractRequest&& addContractRequest,
                     ContractContext& contractContext,
                     const ExecutorConfig& executorConfig)
            : m_contractKey( contractKey )
            , m_driveKey( addContractRequest.m_driveKey )
            , m_executors( std::move(addContractRequest.m_executors) )
            , m_contractContext(contractContext)
            , m_contractConfig(executorConfig)
            {}

    void terminate() override {
        if ( m_task ) {
            m_task->terminate();
        }
    }

public:

    void addContractCall( const CallRequest& request ) override {
        m_batchesManager->addCall( request );
        if ( !m_task ) {
            runTask();
        }
    }

    void removeContract( const RemoveRequest& request ) override {
        if ( m_task ) {
            m_task->terminate();
        } else {
            runTask();
        }
    }

public:

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

public:

    // region virtual machine event handler

    bool onSuperContractCallExecuted( const CallExecutionResult& executionResult ) override {
        if ( !m_task ) {
            return false;
        }

        return m_task->onSuperContractCallExecuted(executionResult);
    }

    // endregion

public:

    // region blockchain event handler

    bool onEndBatchExecutionPublished( const PublishedEndBatchExecutionTransactionInfo& info ) override {
        if ( !m_task || !m_task->onEndBatchExecutionPublished( info )) {
            m_unknownPublishedEndBatchTransactions[info.m_batchIndex] = info;
        }

        return true;
    }

    bool onEndBatchExecutionSingleTransactionPublished(
            const PublishedEndBatchExecutionSingleTransactionInfo& info ) override {
        if ( !m_task ) {
            return false;
        }

        return m_task->onEndBatchExecutionSingleTransactionPublished( info );
    }

    // endregion

public:

    // region message event handler

    bool onEndBatchExecutionOpinionReceived( const EndBatchExecutionOpinion& info ) override {
        if ( !m_task || !m_task->onEndBatchExecutionOpinionReceived( info )) {
            if ( m_batchesManager->batchIndex() <= info.m_batchIndex ) {
                if ( info.isSuccessful()) {
                    m_unknownSuccessfulBatchOpinions[info.m_batchIndex][info.m_executorKey] = info;
                } else {
                    m_unknownUnsuccessfulBatchOpinions[info.m_batchIndex][info.m_executorKey] = info;
                }
            }
        }

        return true;
    }

    // endregion

public:

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

    const DriveKey& driveKey() const override {
        return m_driveKey;
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

    void finishTask() override {

    }

    void notifyNeedsSynchronization( const Hash256& state ) override {

    }

    std::string dbgPeerName() override {
        return m_contractContext.dbgPeerName();
    }

    // endregion

private:

    void runTask() {

        m_task.reset();

        if ( m_contractRemoveRequest ) {
            runRemoveContractTask();
        }
        else if ( m_synchronizationRequest ) {
            runSynchronizationTask();
        }
        else if ( m_batchesManager->hasFormedBatch()) {
            runBatchExecutionTask();
        }
    }

    void runInitializeContractTask() {
    }

    void runRemoveContractTask() {
        _ASSERT( m_contractRemoveRequest )

        m_task = createRemoveContractTask( std::move(*m_contractRemoveRequest), *this );

        m_contractRemoveRequest.reset();

        m_task->run();
    }

    void runSynchronizationTask() {
        _ASSERT( m_synchronizationRequest )

        m_task = createSynchronizationTask( std::move(*m_synchronizationRequest), *this );

        m_synchronizationRequest.reset();

        m_task->run();
    }

    void runBatchExecutionTask() {
        _ASSERT(  m_batchesManager->hasFormedBatch() )

        auto batch = m_batchesManager->popFormedBatch();

        std::map<ExecutorKey, EndBatchExecutionOpinion> successfulEndBatchOpinions;
        auto successfulExecutorsEndBatchOpinionsIt = m_unknownSuccessfulBatchOpinions.find( batch.m_batchIndex );
        if ( successfulExecutorsEndBatchOpinionsIt != m_unknownSuccessfulBatchOpinions.end()) {
            successfulEndBatchOpinions = std::move( successfulExecutorsEndBatchOpinionsIt->second );
            m_unknownSuccessfulBatchOpinions.erase( successfulExecutorsEndBatchOpinionsIt );
        }

        std::map<ExecutorKey, EndBatchExecutionOpinion> unsuccessfulEndBatchOpinions;
        auto unsuccessfulExecutorsEndBatchOpinionsIt = m_unknownUnsuccessfulBatchOpinions.find( batch.m_batchIndex );
        if ( unsuccessfulExecutorsEndBatchOpinionsIt != m_unknownUnsuccessfulBatchOpinions.end()) {
            unsuccessfulEndBatchOpinions = std::move( unsuccessfulExecutorsEndBatchOpinionsIt->second );
            m_unknownUnsuccessfulBatchOpinions.erase( unsuccessfulExecutorsEndBatchOpinionsIt );
        }

        std::optional<PublishedEndBatchExecutionTransactionInfo> publishedInfo;
        auto publishedTransactionInfoIt = m_unknownPublishedEndBatchTransactions.find( batch.m_batchIndex );
        if ( publishedTransactionInfoIt != m_unknownPublishedEndBatchTransactions.end()) {
            publishedInfo = std::move( publishedTransactionInfoIt->second );
            m_unknownPublishedEndBatchTransactions.erase( publishedTransactionInfoIt );
        }

        m_task = createBatchExecutionTask( std::move( batch ), *this, m_contractContext.virtualMachine(),
                                           std::move( successfulEndBatchOpinions ), std::move( unsuccessfulEndBatchOpinions ),
                                           std::move( publishedInfo ));
        m_task->run();
    }

};

std::unique_ptr<Contract> createDefaultContract( const ContractKey& contractKey,
                                                 AddContractRequest&& addContractRequest,
                                                 ContractContext& contractContext,
                                                 const ExecutorConfig& executorConfig ) {
    return std::make_unique<DefaultContract>(contractKey, std::move(addContractRequest), contractContext, executorConfig);
}

}