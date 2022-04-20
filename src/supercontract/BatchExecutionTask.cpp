/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BaseContractTask.h"
#include "ProofOfExecution.h"
#include "CallExecutionEnvironment.h"
#include "utils/Serializer.h"

namespace sirius::contract {

class BatchExecutionTask : public BaseContractTask {

private:

    Batch m_batch;

    VirtualMachine& m_virtualMachine;

    std::vector<CallExecutionInfo> m_callsExecutionInfos;

    std::unique_ptr<CallExecutionEnvironment> m_call;

    std::optional<EndBatchExecutionTransactionInfo> m_successfulEndBatchInfo;
    std::optional<EndBatchExecutionTransactionInfo> m_unsuccessfulEndBatchInfo;

    std::optional<PublishedEndBatchExecutionTransactionInfo> m_publishedEndBatchInfo;

    std::map<ExecutorKey, EndBatchExecutionTransactionInfo> m_otherSuccessfulExecutorEndBatchInfos;
    std::map<ExecutorKey, EndBatchExecutionTransactionInfo> m_otherUnsuccessfulExecutorEndBatchInfos;

    std::optional<boost::asio::high_resolution_timer> m_unsuccessfulExecutionTimer;

    std::optional<boost::asio::high_resolution_timer> m_successfulApprovalExpectationTimer;
    std::optional<boost::asio::high_resolution_timer> m_unsuccessfulApprovalExpectationTimer;

public:

    BatchExecutionTask( Batch&& batch,
                        TaskContext& taskContext,
                        VirtualMachine& virtualMachine,
                        std::map<ExecutorKey, EndBatchExecutionTransactionInfo>&& otherSuccessfulExecutorEndBatchInfos,
                        std::map<ExecutorKey, EndBatchExecutionTransactionInfo>&& otherUnsuccessfulExecutorEndBatchInfos,
                        std::optional<PublishedEndBatchExecutionTransactionInfo>&& publishedEndBatchInfo )
            : BaseContractTask( taskContext )
            , m_batch( std::move( batch ))
            , m_virtualMachine( virtualMachine )
            , m_publishedEndBatchInfo( std::move(publishedEndBatchInfo) )
            , m_otherSuccessfulExecutorEndBatchInfos( std::move( otherSuccessfulExecutorEndBatchInfos ))
            , m_otherUnsuccessfulExecutorEndBatchInfos( std::move( otherUnsuccessfulExecutorEndBatchInfos ))
            {}

    void run() override {
        m_taskContext.threadManager().execute( [this] {
            m_taskContext.storageBridge().initiateModifications( m_taskContext.driveKey(), m_batch.m_batchIndex );
        } );
    }

    void terminate() override {
        m_unsuccessfulExecutionTimer.reset();
        m_successfulApprovalExpectationTimer.reset();
        m_unsuccessfulApprovalExpectationTimer.reset();

        m_taskContext.threadManager().execute( [this] {
            m_taskContext.onTaskFinished();
        } );
    }

public:

    // region virtual machine event handler

    bool onSuperContractCallExecuted( const CallExecutionResult& executionResult ) override {
        if ( !m_call || m_call->callId() != executionResult.m_callId ) {
            return false;
        }

        m_call->setCallExecutionResult( executionResult );

        m_taskContext.threadManager().execute([=, this] {
            m_taskContext.storageBridge().applyStorageModifications( m_taskContext.driveKey(), m_batch.m_batchIndex,
                                                                     executionResult.m_success );
        } );

        return true;
    }

    // endregion

public:

    // region storage bridge event handler

    bool onInitiatedModifications( uint64_t batchIndex ) override {
        if ( batchIndex != m_batch.m_batchIndex ) {
            return false;
        }

        executeNextCall();

        return true;
    }

    bool onAppliedSandboxStorageModifications( uint64_t batchIndex, bool success, int64_t sandboxSizeDelta,
                                               int64_t stateSizeDelta ) override {
        if ( m_batch.m_batchIndex != batchIndex ) {
            return false;
        }

        const auto& executionResult = m_call->callExecutionResult();

        m_callsExecutionInfos.push_back( CallExecutionInfo{
                executionResult.m_callId,
                SuccessfulBatchCallInfo{
                        success,
                        sandboxSizeDelta,
                        stateSizeDelta,
                },
                {
                        CallExecutorParticipation{
                                executionResult.m_scConsumed,
                                executionResult.m_smConsumed
                        }
                }
        } );

        executeNextCall();

        return true;
    }

    bool onRootHashEvaluated( uint64_t batchIndex, const Hash256& rootHash,
                              uint64_t usedDriveSize, uint64_t metaFilesSize, uint64_t fileStructureSize ) override {
        if( m_batch.m_batchIndex != batchIndex ) {
            return false;
        }

        m_taskContext.threadManager().execute( [=, this] {
            formSuccessfulEndBatchInfo( rootHash, usedDriveSize, metaFilesSize, fileStructureSize );
        } );

        return true;
    }

    bool onAppliedStorageModifications( uint64_t batchIndex ) override {
        if( m_batch.m_batchIndex != batchIndex ) {
            return false;
        }

        m_taskContext.threadManager().execute( [this] {
            m_taskContext.onTaskFinished();
        } );

        return true;
    }

    // endregion

public:

    // region messenger event handler

    bool onEndBatchExecutionOpinionReceived( const EndBatchExecutionTransactionInfo& info ) override {
        if ( info.m_batchIndex != m_batch.m_batchIndex ) {
            return false;
        }

        if ( info.isSuccessful()) {
            m_otherSuccessfulExecutorEndBatchInfos[info.m_executorKeys.front()] = info;
        } else {
            m_otherUnsuccessfulExecutorEndBatchInfos[info.m_executorKeys.front()] = info;
        }

        return true;
    }

    // endregion

public:

    // region blockchain event handler

    bool onEndBatchExecutionPublished( const PublishedEndBatchExecutionTransactionInfo& info ) override {
        if ( info.m_batchIndex != m_batch.m_batchIndex ) {
            return false;
        }

        m_publishedEndBatchInfo = info;

        if ( m_successfulEndBatchInfo ) {
            processPublishedEndBatch();
        } else {
            // We are not able to process the transaction yet, we will do it as soon as the batch will be executed
        }

        return true;
    }

    // endregion

private:

    void formSuccessfulEndBatchInfo( const Hash256& rootHash,
                                     uint64_t usedDriveSize, uint64_t metaFilesSize, uint64_t fileStructureSize ) {
        m_successfulEndBatchInfo = EndBatchExecutionTransactionInfo();
        m_successfulEndBatchInfo->m_batchIndex = m_batch.m_batchIndex;
        m_successfulEndBatchInfo->m_contractKey = m_taskContext.contractKey();
        m_successfulEndBatchInfo->m_executorKeys.emplace_back( m_taskContext.keyPair().publicKey());

        m_successfulEndBatchInfo->m_successfulBatchInfo = SuccessfulBatchInfo{rootHash, usedDriveSize, metaFilesSize,
                                                                              fileStructureSize, {}};

        m_successfulEndBatchInfo->m_callsExecutionInfo = std::move( m_callsExecutionInfos );

        for ( const auto& executor: m_taskContext.executors()) {
            auto serializedInfo = serialize( *m_successfulEndBatchInfo );
            m_taskContext.messenger().sendMessage( executor, serializedInfo );
        }

        m_unsuccessfulExecutionTimer = m_taskContext.threadManager().startTimer(
                m_taskContext.contractConfig().unsuccessfulApprovalDelayMs(), [this] {
                    onUnsuccessfulExecutionTimerExpiration();
                } );

        std::erase_if( m_otherSuccessfulExecutorEndBatchInfos, [this]( const auto& item ) {
            return !validateOtherBatchInfo( item );
        } );

        std::erase_if( m_otherUnsuccessfulExecutorEndBatchInfos, [this]( const auto& item ) {
            return !validateOtherBatchInfo( item );
        } );

        if ( m_publishedEndBatchInfo ) {
            processPublishedEndBatch();
        }
        else {
            checkEndBatchTransactionReadiness();
        }
    }

    void processPublishedEndBatch() {

        _ASSERT( m_publishedEndBatchInfo )
        _ASSERT( m_successfulEndBatchInfo )

        bool batchIsSuccessful = m_publishedEndBatchInfo->isSuccessful();

        if ( batchIsSuccessful && *m_publishedEndBatchInfo->m_driveState !=
                                  m_successfulEndBatchInfo->m_successfulBatchInfo->m_rootHash ) {
            // The received result differs from the common one, synchronization is needed
            m_taskContext.notifyNeedsSynchronization( *m_publishedEndBatchInfo->m_driveState );

            m_taskContext.threadManager().execute( [this] {
                m_taskContext.onTaskFinished();
            } );

        } else {

            m_taskContext.threadManager().execute( [=, this] {
                m_taskContext.storageBridge().applyStorageModifications( m_taskContext.driveKey(), m_batch.m_batchIndex,
                                                                         batchIsSuccessful );
            } );

            const auto& cosigners = m_publishedEndBatchInfo->m_cosigners;
            if ( std::find( cosigners.begin(), cosigners.end(), m_taskContext.keyPair().publicKey()) ==
                 cosigners.end()) {
                // TODO PoEx
                EndBatchExecutionSingleTransactionInfo singleTx = {m_taskContext.contractKey(), m_batch.m_batchIndex,
                                                                   {}};
                m_taskContext.executorEventHandler().endBatchSingleTransactionIsReady( singleTx );
            }
        }
    }

    bool validateOtherBatchInfo( const EndBatchExecutionTransactionInfo& other ) {

        bool otherSuccessfulBatch = other.m_successfulBatchInfo.has_value();

        if ( otherSuccessfulBatch ) {
            const auto& otherSuccessfulBatchInfo = *other.m_successfulBatchInfo;
            const auto& successfulBatchInfo = *m_successfulEndBatchInfo->m_successfulBatchInfo;

//            if (otherSuccessfulBatchInfo.m_verificationInfo != successfulBatchInfo.m_verificationInfo) {
//                return false;
//            }

            if ( otherSuccessfulBatchInfo.m_rootHash != successfulBatchInfo.m_rootHash ) {
                return false;
            }

            if ( otherSuccessfulBatchInfo.m_usedDriveSize != successfulBatchInfo.m_usedDriveSize ) {
                return false;
            }

            if ( otherSuccessfulBatchInfo.m_metaFilesSize != successfulBatchInfo.m_metaFilesSize ) {
                return false;
            }

            if ( otherSuccessfulBatchInfo.m_fileStructureSize != successfulBatchInfo.m_fileStructureSize ) {
                return false;
            }
        }

        if ( other.m_callsExecutionInfo.size() != m_successfulEndBatchInfo->m_callsExecutionInfo.size()) {
            return false;
        }

        auto otherCallIt = other.m_callsExecutionInfo.begin();
        auto callIt = m_successfulEndBatchInfo->m_callsExecutionInfo.begin();
        for ( ; otherCallIt != other.m_callsExecutionInfo.end(); otherCallIt++, callIt++ ) {
            if ( otherCallIt->m_callId != callIt->m_callId ) {
                return false;
            }

            if ( otherSuccessfulBatch ) {
                const auto& otherSuccessfulBatchInfo = *otherCallIt->m_callExecutionInfo;
                const auto& successfulBatchInfo = *callIt->m_callExecutionInfo;

                if ( otherSuccessfulBatchInfo.m_callExecutionSuccess != successfulBatchInfo.m_callExecutionSuccess ) {
                    return false;
                }

                if ( otherSuccessfulBatchInfo.m_callSandboxSizeDelta !=
                     successfulBatchInfo.m_callSandboxSizeDelta ) {
                    return false;
                }

                if ( otherSuccessfulBatchInfo.m_callStateSizeDelta !=
                     successfulBatchInfo.m_callStateSizeDelta ) {
                    return false;
                }
            }
        }

        return true;
    }

    void checkEndBatchTransactionReadiness() {
        _ASSERT( m_successfulEndBatchInfo )

        if ( m_successfulEndBatchInfo &&
             2 * m_otherSuccessfulExecutorEndBatchInfos.size() >= 3 * (m_taskContext.executors().size() + 1)) {
            // Enough signatures for successful batch
            auto tx = createMultisigTransactionInfo( *m_successfulEndBatchInfo,
                                                     m_otherSuccessfulExecutorEndBatchInfos );

            m_taskContext.threadManager().startTimer(
                    m_taskContext.contractConfig().executorConfig().successfulExecutionDelayMs(),
                    [this, tx = std::move( tx )] {
                        sendEndBatchTransaction( tx );
                    } );
        } else if ( m_unsuccessfulEndBatchInfo &&
                    2 * m_otherUnsuccessfulExecutorEndBatchInfos.size() >= 3 * (m_taskContext.executors().size() + 1)) {
            auto tx = createMultisigTransactionInfo( *m_unsuccessfulEndBatchInfo,
                                                     m_otherUnsuccessfulExecutorEndBatchInfos );
            m_taskContext.threadManager().startTimer(
                    m_taskContext.contractConfig().executorConfig().unsuccessfulExecutionDelayMs(),
                    [this, tx = std::move( tx )] {
                        sendEndBatchTransaction( tx );
                    } );
        }
    }

    EndBatchExecutionTransactionInfo
    createMultisigTransactionInfo( const EndBatchExecutionTransactionInfo& transactionInfo,
                                   const std::map<ExecutorKey, EndBatchExecutionTransactionInfo>& otherTransactionInfos ) {
        auto multisigTransactionInfo = transactionInfo;

        for ( const auto&[_, otherInfo]: otherTransactionInfos ) {
            multisigTransactionInfo.m_executorKeys.push_back( otherInfo.m_executorKeys.front());
            multisigTransactionInfo.m_signatures.push_back( otherInfo.m_signatures.front());
            multisigTransactionInfo.m_proofs.push_back( otherInfo.m_proofs.front());

            _ASSERT( multisigTransactionInfo.m_callsExecutionInfo.size() == otherInfo.m_callsExecutionInfo.size())
            auto txCallIt = multisigTransactionInfo.m_callsExecutionInfo.begin();
            auto otherCallIt = multisigTransactionInfo.m_callsExecutionInfo.begin();
            for ( ; txCallIt != transactionInfo.m_callsExecutionInfo.end(); txCallIt++, otherCallIt++ ) {
                _ASSERT( txCallIt->m_callId == otherCallIt->m_callId )
                txCallIt->m_executorsParticipation.push_back( otherCallIt->m_executorsParticipation.front());
            }
        }

        return multisigTransactionInfo;
    }

    void sendEndBatchTransaction( const EndBatchExecutionTransactionInfo& transactionInfo ) {
        m_taskContext.executorEventHandler().endBatchTransactionIsReady( transactionInfo );
    }

    void executeNextCall() {
        if ( !m_batch.m_callRequests.empty()) {
            m_call = std::make_unique<CallExecutionEnvironment>( std::move( m_batch.m_callRequests.front()));
            m_batch.m_callRequests.pop_front();

            m_taskContext.threadManager().execute( [this] {
                m_virtualMachine.execute( m_call->callRequest());
            } );
        } else {
            computeProofOfExecution();

            m_taskContext.threadManager().execute( [this] {
                m_taskContext.storageBridge().evaluateRootHash( m_taskContext.driveKey(), m_batch.m_batchIndex );
            } );
        }
    }

    void onUnsuccessfulExecutionTimerExpiration() {

        _ASSERT( m_successfulEndBatchInfo )
        _ASSERT( !m_unsuccessfulEndBatchInfo )

        m_unsuccessfulEndBatchInfo = m_successfulEndBatchInfo;

        m_unsuccessfulEndBatchInfo->m_successfulBatchInfo.reset();
        for ( auto& callInfo: m_unsuccessfulEndBatchInfo->m_callsExecutionInfo ) {
            callInfo.m_callExecutionInfo.reset();
        }

        for ( const auto& executor: m_taskContext.executors()) {
            auto serializedInfo = serialize( *m_unsuccessfulEndBatchInfo );
            m_taskContext.messenger().sendMessage( executor,  serializedInfo );
        }
    }

    void computeProofOfExecution() {

    }
};

std::unique_ptr<BaseContractTask> createBatchExecutionTask( Batch&& batch,
                                                            TaskContext& taskContext,
                                                            VirtualMachine& virtualMachine,
                                                            std::map<ExecutorKey, EndBatchExecutionTransactionInfo>&& otherSuccessfulExecutorEndBatchInfos,
                                                            std::map<ExecutorKey, EndBatchExecutionTransactionInfo>&& otherUnsuccessfulExecutorEndBatchInfos,
                                                            std::optional<PublishedEndBatchExecutionTransactionInfo>&& publishedEndBatchInfo ) {
    return std::make_unique<BatchExecutionTask>( std::move( batch ), taskContext, virtualMachine,
                                                 std::move( otherSuccessfulExecutorEndBatchInfos ),
                                                 std::move( otherUnsuccessfulExecutorEndBatchInfos ),
                                                 std::move( publishedEndBatchInfo ));
}

}