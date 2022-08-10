/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "Contract.h"
#include "BatchesManager.h"
#include "ContractEnvironment.h"
#include "BaseContractTask.h"
#include "supercontract/ThreadManager.h"
#include "ProofOfExecution.h"
#include "ContractConfig.h"
#include "ExecutorEnvironment.h"
#include "TaskRequests.h"
#include "DefaultBatchesManager.h"
#include "DebugInfo.h"

namespace sirius::contract {

class DefaultContract : public Contract, public ContractEnvironment {

private:

    ContractKey                                 m_contractKey;

    DriveKey                                    m_driveKey;
    std::set<ExecutorKey>                       m_executors;

    uint64_t                                    m_automaticExecutionsSCLimit;
    uint64_t                                    m_automaticExecutionsSMLimit;

    ExecutorEnvironment&                        m_executorEnvironment;
    ContractConfig                              m_contractConfig;

    ProofOfExecution                            m_proofOfExecution;

    const DebugInfo                             m_dbgInfo;

    // Requests
    std::unique_ptr<BaseBatchesManager>         m_batchesManager;
    std::optional<SynchronizationRequest>       m_synchronizationRequest;
    std::optional<RemoveRequest>                m_contractRemoveRequest;

    std::unique_ptr<BaseContractTask>           m_task;

    std::map<uint64_t, std::map<ExecutorKey, EndBatchExecutionOpinion>> m_unknownSuccessfulBatchOpinions;
    std::map<uint64_t, std::map<ExecutorKey, EndBatchExecutionOpinion>> m_unknownUnsuccessfulBatchOpinions;
    std::map<uint64_t, PublishedEndBatchExecutionTransactionInfo> m_unknownPublishedEndBatchTransactions;

public:

    DefaultContract( const ContractKey& contractKey,
                     AddContractRequest&& addContractRequest,
                     ExecutorEnvironment& contractContext,
                     const DebugInfo& debugInfo )
            : m_contractKey( contractKey )
            , m_driveKey( addContractRequest.m_driveKey )
            , m_executors( std::move(addContractRequest.m_executors) )
            , m_automaticExecutionsSCLimit( addContractRequest.m_automaticExecutionsSCLimit )
            , m_automaticExecutionsSMLimit( addContractRequest.m_automaticExecutionsSMLimit )
            , m_executorEnvironment( contractContext)
            , m_contractConfig()
            , m_dbgInfo( debugInfo )
            , m_batchesManager( std::make_unique<DefaultBatchesManager>( addContractRequest.m_batchesExecuted, *this, m_executorEnvironment, m_dbgInfo ) ) {
        runInitializeContractTask(std::move(addContractRequest));
    }

    void terminate() override {

        DBG_MAIN_THREAD

        if ( m_task ) {
            m_task->terminate();
        }
    }

    void addContractCall( const CallRequest& request ) override {

        DBG_MAIN_THREAD

        m_batchesManager->addCall( request );
        if ( !m_task ) {
            runTask();
        }
    }

    void removeContract( const RemoveRequest& request ) override {

        DBG_MAIN_THREAD

        if ( m_task ) {
            m_task->terminate();
        } else {
            runTask();
        }
    }

    void setExecutors( std::set<ExecutorKey>&& executors ) override {

        DBG_MAIN_THREAD

        m_executors = std::move(executors);
    }

    void addBlockInfo( const Block& block ) override {

        DBG_MAIN_THREAD

        m_batchesManager->addBlockInfo( block );
    }

    void setAutomaticExecutionsEnabledSince( const std::optional<uint64_t>& blockHeight ) override {

        DBG_MAIN_THREAD

        m_batchesManager->setAutomaticExecutionsEnabledSince(blockHeight);
    }

public:

    // region storage event handler

    bool onInitiatedModifications( uint64_t batchIndex ) override {

        DBG_MAIN_THREAD

        if ( !m_task ) {
            return false;
        }

        return m_task->onInitiatedModifications( batchIndex );
    }

    bool onAppliedSandboxStorageModifications( uint64_t batchIndex, bool success, int64_t sandboxSizeDelta, int64_t stateSizeDelta ) override {

        DBG_MAIN_THREAD

        if ( !m_task ) {
            return false;
        }

        return m_task->onAppliedSandboxStorageModifications(batchIndex, success, sandboxSizeDelta, stateSizeDelta);
    }

    bool
    onStorageHashEvaluated( uint64_t batchIndex, const StorageHash& storageHash, uint64_t usedDriveSize, uint64_t metaFilesSize,
                            uint64_t fileStructureSize ) override {

        DBG_MAIN_THREAD

        if ( !m_task ) {
            return false;
        }

        return m_task->onStorageHashEvaluated( batchIndex, storageHash, usedDriveSize, metaFilesSize, fileStructureSize );
    }

    // endregion

public:

    // region virtual machine event handler

    bool onSuperContractCallExecuted( const CallExecutionResult& executionResult ) override {

        DBG_MAIN_THREAD

        if ( m_batchesManager->onSuperContractCallExecuted(executionResult) ) {
            return true;
        }

        if (m_task && m_task->onSuperContractCallExecuted(executionResult)) {
            return true;
        }

        return false;
    }

    // endregion

public:

    // region blockchain event handler

    bool onEndBatchExecutionPublished( const PublishedEndBatchExecutionTransactionInfo& info ) override {

        DBG_MAIN_THREAD

        while ( !m_unknownSuccessfulBatchOpinions.empty()
                && m_unknownSuccessfulBatchOpinions.begin()->first <= info.m_batchIndex ) {
            m_unknownSuccessfulBatchOpinions.erase( m_unknownSuccessfulBatchOpinions.begin());
        }

        while ( !m_unknownUnsuccessfulBatchOpinions.empty()
                && m_unknownUnsuccessfulBatchOpinions.begin()->first <= info.m_batchIndex ) {
            m_unknownUnsuccessfulBatchOpinions.erase( m_unknownUnsuccessfulBatchOpinions.begin());
        }

        if ( !m_task || !m_task->onEndBatchExecutionPublished( info )) {
            m_unknownPublishedEndBatchTransactions[info.m_batchIndex] = info;
        }

        return true;
    }

    bool onEndBatchExecutionFailed( const FailedEndBatchExecutionTransactionInfo& info ) override {

        DBG_MAIN_THREAD

        if ( !m_task ) {
            return false;
        }

        return m_task->onEndBatchExecutionFailed(info);
    }

    bool onStorageSynchronized( uint64_t batchIndex ) override {

        DBG_MAIN_THREAD

        m_batchesManager->onStorageSynchronized( batchIndex );

        if ( !m_task ) {
            return false;
        }

        m_task->onStorageSynchronized( batchIndex );

        // TODO should we always return true?
        return true;
    }

    // endregion

public:

    // region message event handler

    bool onEndBatchExecutionOpinionReceived( const EndBatchExecutionOpinion& info ) override {

        DBG_MAIN_THREAD

        if ( !m_task || !m_task->onEndBatchExecutionOpinionReceived( info )) {
            if ( info.isSuccessful()) {
                m_unknownSuccessfulBatchOpinions[info.m_batchIndex][info.m_executorKey] = info;
            } else {
                m_unknownUnsuccessfulBatchOpinions[info.m_batchIndex][info.m_executorKey] = info;
            }
        }

        return true;
    }

    // endregion

public:

    // region task context

    const ContractKey& contractKey() const override {

        DBG_MAIN_THREAD

        return m_contractKey;
    }

    const std::set<ExecutorKey>& executors() const override {

        DBG_MAIN_THREAD

        return m_executors;
    }

    const DriveKey& driveKey() const override {

        DBG_MAIN_THREAD

        return m_driveKey;
    }

    uint64_t automaticExecutionsSCLimit() const override {

        DBG_MAIN_THREAD

        return m_automaticExecutionsSCLimit;
    }

    uint64_t automaticExecutionsSMLimit() const override {

        DBG_MAIN_THREAD

        return m_automaticExecutionsSMLimit;
    }

    const ContractConfig& contractConfig() const override {

        DBG_MAIN_THREAD

        return m_contractConfig;
    }

    void finishTask() override {

        DBG_MAIN_THREAD

        runTask();
    }

    void addSynchronizationTask( const SynchronizationRequest& request ) override {

        DBG_MAIN_THREAD

        m_synchronizationRequest = request;
    }

    // endregion

private:

    void runTask() {

        DBG_MAIN_THREAD

        m_task.reset();

        if ( m_contractRemoveRequest ) {
            runRemoveContractTask();
        }
        else if ( m_synchronizationRequest ) {
            runSynchronizationTask();
        }
        else if ( m_batchesManager->hasNextBatch() ) {
            runBatchExecutionTask();
        }
    }

    void runInitializeContractTask( AddContractRequest&& request ) {

        DBG_MAIN_THREAD

        m_task = createInitContractTask( std::move(request), *this, m_executorEnvironment, m_dbgInfo );

        m_contractRemoveRequest.reset();

        m_task->run();
    }

    void runRemoveContractTask() {

        DBG_MAIN_THREAD

        _ASSERT( m_contractRemoveRequest )

        m_task = createRemoveContractTask( std::move(*m_contractRemoveRequest), *this, m_executorEnvironment, m_dbgInfo );

        m_contractRemoveRequest.reset();

        m_task->run();
    }

    void runSynchronizationTask() {

        DBG_MAIN_THREAD

        _ASSERT( m_synchronizationRequest )

        m_task = createSynchronizationTask( std::move(*m_synchronizationRequest), *this, m_executorEnvironment, m_dbgInfo );

        m_synchronizationRequest.reset();

        m_task->run();
    }

    void runBatchExecutionTask() {

        DBG_MAIN_THREAD

        _ASSERT(  m_batchesManager->hasNextBatch() )

        auto batch = m_batchesManager->nextBatch();

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

        m_task = createBatchExecutionTask( std::move( batch ), *this, m_executorEnvironment,
                                           std::move( successfulEndBatchOpinions ),
                                           std::move( unsuccessfulEndBatchOpinions ),
                                           std::move( publishedInfo ),
                                           m_dbgInfo );
        m_task->run();
    }

};

std::unique_ptr<Contract> createDefaultContract( const ContractKey& contractKey,
                                                 AddContractRequest&& addContractRequest,
                                                 ExecutorEnvironment& contractContext,
                                                 const DebugInfo& debugInfo ) {
    return std::make_unique<DefaultContract>( contractKey, std::move(addContractRequest), contractContext, debugInfo );
}

}