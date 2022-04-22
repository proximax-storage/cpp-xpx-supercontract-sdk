/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BaseContractTask.h"
#include "ProofOfExecution.h"
#include "CallExecutionEnvironment.h"
#include "utils/Serializer.h"
#include "Messages.h"

namespace sirius::contract {

class BatchExecutionTask : public BaseContractTask {

private:

    Batch                                                               m_batch;

    VirtualMachine&                                                     m_virtualMachine;

    std::vector<CallExecutionOpinion>                                   m_callsExecutionOpinions;

    std::unique_ptr<CallExecutionEnvironment>                           m_call;

    std::optional<EndBatchExecutionOpinion>                             m_successfulEndBatchOpinion;
    std::optional<EndBatchExecutionOpinion>                             m_unsuccessfulEndBatchOpinion;

    std::optional<PublishedEndBatchExecutionTransactionInfo>            m_publishedEndBatchInfo;

    std::map<ExecutorKey, EndBatchExecutionOpinion>                     m_otherSuccessfulExecutorEndBatchOpinions;
    std::map<ExecutorKey, EndBatchExecutionOpinion>                     m_otherUnsuccessfulExecutorEndBatchOpinions;

    std::optional<boost::asio::high_resolution_timer>                   m_unsuccessfulExecutionTimer;

    std::optional<boost::asio::high_resolution_timer>                   m_successfulApprovalExpectationTimer;
    std::optional<boost::asio::high_resolution_timer>                   m_unsuccessfulApprovalExpectationTimer;

public:

    BatchExecutionTask( Batch&& batch,
                        TaskContext& taskContext,
                        VirtualMachine& virtualMachine,
                        std::map<ExecutorKey, EndBatchExecutionOpinion>&& otherSuccessfulExecutorEndBatchOpinions,
                        std::map<ExecutorKey, EndBatchExecutionOpinion>&& otherUnsuccessfulExecutorEndBatchOpinions,
                        std::optional<PublishedEndBatchExecutionTransactionInfo>&& publishedEndBatchInfo )
            : BaseContractTask( taskContext )
            , m_batch( std::move( batch ))
            , m_virtualMachine( virtualMachine )
            , m_publishedEndBatchInfo( std::move(publishedEndBatchInfo) )
            , m_otherSuccessfulExecutorEndBatchOpinions( std::move( otherSuccessfulExecutorEndBatchOpinions ))
            , m_otherUnsuccessfulExecutorEndBatchOpinions( std::move( otherSuccessfulExecutorEndBatchOpinions ))
            {}

    void run() override {
        m_taskContext.storageBridge().initiateModifications( m_taskContext.driveKey(),
                                                             m_batch.m_batchIndex );
    }

    void terminate() override {
        m_unsuccessfulExecutionTimer.reset();
        m_successfulApprovalExpectationTimer.reset();
        m_unsuccessfulApprovalExpectationTimer.reset();

        m_taskContext.finishTask();
    }

public:

    // region virtual machine event handler

    bool onSuperContractCallExecuted( const CallExecutionResult& executionResult ) override {
        if ( !m_call || m_call->callId() != executionResult.m_callId ) {
            return false;
        }

        m_call->setCallExecutionResult( executionResult );

        m_taskContext.storageBridge().applyStorageModifications( m_taskContext.driveKey(), m_batch.m_batchIndex,
                                                                 executionResult.m_success );

        return true;
    }

    // endregion

public:

    // region message event handler

    bool onEndBatchExecutionOpinionReceived( const EndBatchExecutionOpinion& opinion ) override {
        if (opinion.m_batchIndex != m_batch.m_batchIndex) {
            return false;
        }

        if ( !m_successfulEndBatchOpinion || validateOtherBatchInfo(opinion) ) {
            if ( opinion.isSuccessful()) {
                m_otherSuccessfulExecutorEndBatchOpinions[opinion.m_executorKey] = opinion;
            } else {
                m_otherUnsuccessfulExecutorEndBatchOpinions[opinion.m_executorKey] = opinion;
            }
        }

        checkEndBatchTransactionReadiness();

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

        m_callsExecutionOpinions.push_back( CallExecutionOpinion{
                executionResult.m_callId,
                SuccessfulBatchCallInfo{
                        success,
                        sandboxSizeDelta,
                        stateSizeDelta,
                },
                CallExecutorParticipation{
                        executionResult.m_scConsumed,
                        executionResult.m_smConsumed
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

        formSuccessfulEndBatchOpinion( rootHash, usedDriveSize, metaFilesSize, fileStructureSize );

        return true;
    }

    bool onAppliedStorageModifications( uint64_t batchIndex ) override {
        if( m_batch.m_batchIndex != batchIndex ) {
            return false;
        }

        m_taskContext.finishTask();

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

        if ( m_successfulEndBatchOpinion ) {
            processPublishedEndBatch();
        } else {
            // We are not able to process the transaction yet, we will do it as soon as the batch will be executed
        }

        return true;
    }

    // endregion

private:

    void formSuccessfulEndBatchOpinion( const Hash256& rootHash,
                                        uint64_t usedDriveSize, uint64_t metaFilesSize, uint64_t fileStructureSize ) {
        m_successfulEndBatchOpinion = EndBatchExecutionOpinion();
        m_successfulEndBatchOpinion->m_batchIndex = m_batch.m_batchIndex;
        m_successfulEndBatchOpinion->m_contractKey = m_taskContext.contractKey();
        m_successfulEndBatchOpinion->m_executorKey = m_taskContext.keyPair().publicKey();

        m_successfulEndBatchOpinion->m_successfulBatchInfo = SuccessfulBatchInfo{rootHash, usedDriveSize, metaFilesSize,
                                                                                 fileStructureSize, {}};

        m_successfulEndBatchOpinion->m_callsExecutionInfo = std::move( m_callsExecutionOpinions );

        for ( const auto& executor: m_taskContext.executors()) {
            auto serializedInfo = utils::serialize( *m_successfulEndBatchOpinion );
            m_taskContext.messenger().sendMessage( executor, serializedInfo );
        }

        m_unsuccessfulExecutionTimer = m_taskContext.threadManager().startTimer(
                m_taskContext.contractConfig().unsuccessfulApprovalDelayMs(), [this] {
                    onUnsuccessfulExecutionTimerExpiration();
                } );

        std::erase_if( m_otherSuccessfulExecutorEndBatchOpinions, [this]( const auto& item ) {
            return !validateOtherBatchInfo( item.second );
        } );

        std::erase_if( m_otherUnsuccessfulExecutorEndBatchOpinions, [this]( const auto& item ) {
            return !validateOtherBatchInfo( item.second );
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
        _ASSERT( m_successfulEndBatchOpinion )

        bool batchIsSuccessful = m_publishedEndBatchInfo->isSuccessful();

        if ( batchIsSuccessful && *m_publishedEndBatchInfo->m_driveState !=
                                  m_successfulEndBatchOpinion->m_successfulBatchInfo->m_rootHash ) {
            // The received result differs from the common one, synchronization is needed
            m_taskContext.addSynchronizationTask( *m_publishedEndBatchInfo->m_driveState );

            m_taskContext.finishTask();

        } else {

            m_taskContext.storageBridge().applyStorageModifications( m_taskContext.driveKey(),
                                                                     m_batch.m_batchIndex,
                                                                     batchIsSuccessful );

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

    bool validateOtherBatchInfo( const EndBatchExecutionOpinion& other ) {

        _ASSERT( m_successfulEndBatchOpinion )

        bool otherSuccessfulBatch = other.m_successfulBatchInfo.has_value();

        if ( otherSuccessfulBatch ) {
            const auto& otherSuccessfulBatchInfo = *other.m_successfulBatchInfo;
            const auto& successfulBatchInfo = *m_successfulEndBatchOpinion->m_successfulBatchInfo;

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

        if ( other.m_callsExecutionInfo.size() != m_successfulEndBatchOpinion->m_callsExecutionInfo.size()) {
            return false;
        }

        auto otherCallIt = other.m_callsExecutionInfo.begin();
        auto callIt = m_successfulEndBatchOpinion->m_callsExecutionInfo.begin();
        for ( ; otherCallIt != other.m_callsExecutionInfo.end(); otherCallIt++, callIt++ ) {
            if ( otherCallIt->m_callId != callIt->m_callId ) {
                return false;
            }

            if ( otherSuccessfulBatch ) {
                const auto& otherSuccessfulCallInfo = *otherCallIt->m_successfulCallExecutionInfo;
                const auto& successfulCallInfo = *callIt->m_successfulCallExecutionInfo;

                if ( otherSuccessfulCallInfo.m_callExecutionSuccess != successfulCallInfo.m_callExecutionSuccess ) {
                    return false;
                }

                if ( otherSuccessfulCallInfo.m_callSandboxSizeDelta !=
                     successfulCallInfo.m_callSandboxSizeDelta ) {
                    return false;
                }

                if ( otherSuccessfulCallInfo.m_callStateSizeDelta !=
                     successfulCallInfo.m_callStateSizeDelta ) {
                    return false;
                }
            }
        }

        return true;
    }

    void checkEndBatchTransactionReadiness() {
        _ASSERT( m_successfulEndBatchOpinion )

        if ( m_successfulEndBatchOpinion &&
             2 * m_otherSuccessfulExecutorEndBatchOpinions.size() >= 3 * (m_taskContext.executors().size() + 1)) {
            // Enough signatures for successful batch
            auto tx = createMultisigTransactionInfo( *m_successfulEndBatchOpinion,
                                                     std::move( m_otherSuccessfulExecutorEndBatchOpinions ));

            m_taskContext.threadManager().startTimer(
                    m_taskContext.contractConfig().executorConfig().successfulExecutionDelayMs(),
                    [this, tx = std::move( tx )] {
                        sendEndBatchTransaction( tx );
                    } );
        } else if ( m_unsuccessfulEndBatchOpinion &&
                    2 * m_otherUnsuccessfulExecutorEndBatchOpinions.size() >=
                    3 * (m_taskContext.executors().size() + 1)) {
            auto tx = createMultisigTransactionInfo( *m_unsuccessfulEndBatchOpinion,
                                                     std::move( m_otherUnsuccessfulExecutorEndBatchOpinions ));
            m_taskContext.threadManager().startTimer(
                    m_taskContext.contractConfig().executorConfig().unsuccessfulExecutionDelayMs(),
                    [this, tx = std::move( tx )] {
                        sendEndBatchTransaction( tx );
                    } );
        }
    }

    EndBatchExecutionTransactionInfo
    createMultisigTransactionInfo( const EndBatchExecutionOpinion& transactionOpinion,
                                   std::map<ExecutorKey, EndBatchExecutionOpinion>&& otherTransactionOpinions ) {
        EndBatchExecutionTransactionInfo multisigTransactionInfo;

        // Fill common information
        multisigTransactionInfo.m_contractKey = transactionOpinion.m_contractKey;
        multisigTransactionInfo.m_batchIndex = transactionOpinion.m_batchIndex;
        multisigTransactionInfo.m_successfulBatchInfo = transactionOpinion.m_successfulBatchInfo;

        for ( const auto& call: multisigTransactionInfo.m_callsExecutionInfo ) {
            CallExecutionInfo callInfo;
            callInfo.m_callId = call.m_callId;
            callInfo.m_callExecutionInfo = call.m_callExecutionInfo;
            multisigTransactionInfo.m_callsExecutionInfo.push_back( callInfo );
        }

        otherTransactionOpinions[m_taskContext.keyPair().publicKey()] = transactionOpinion;

        for ( const auto&[_, otherOpinion]: otherTransactionOpinions ) {
            multisigTransactionInfo.m_executorKeys.push_back( otherOpinion.m_executorKey );
            multisigTransactionInfo.m_signatures.push_back( otherOpinion.m_signature );
            multisigTransactionInfo.m_proofs.push_back( otherOpinion.m_proof );

            _ASSERT( multisigTransactionInfo.m_callsExecutionInfo.size() == otherOpinion.m_callsExecutionInfo.size())
            auto txCallIt = multisigTransactionInfo.m_callsExecutionInfo.begin();
            auto otherCallIt = otherOpinion.m_callsExecutionInfo.begin();
            for ( ; txCallIt != multisigTransactionInfo.m_callsExecutionInfo.end(); txCallIt++, otherCallIt++ ) {
                _ASSERT( txCallIt->m_callId == otherCallIt->m_callId )
                txCallIt->m_executorsParticipation.push_back( otherCallIt->m_executorParticipation );
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

            m_virtualMachine.execute( m_call->callRequest());
        } else {
            computeProofOfExecution();

            m_taskContext.storageBridge().evaluateRootHash( m_taskContext.driveKey(), m_batch.m_batchIndex );
        }
    }

    void onUnsuccessfulExecutionTimerExpiration() {

        _ASSERT( m_successfulEndBatchOpinion )
        _ASSERT( !m_unsuccessfulEndBatchOpinion )

        m_unsuccessfulEndBatchOpinion = m_successfulEndBatchOpinion;

        m_unsuccessfulEndBatchOpinion->m_successfulBatchInfo.reset();
        for ( auto& callInfo: m_unsuccessfulEndBatchOpinion->m_callsExecutionInfo ) {
            callInfo.m_successfulCallExecutionInfo.reset();
        }

        for ( const auto& executor: m_taskContext.executors()) {
            auto serializedInfo = utils::serialize( *m_unsuccessfulEndBatchOpinion );
            m_taskContext.messenger().sendMessage( executor,  serializedInfo );
        }
    }

    void computeProofOfExecution() {

    }
};

std::unique_ptr<BaseContractTask> createBatchExecutionTask( Batch&& batch,
                                                            TaskContext& taskContext,
                                                            VirtualMachine& virtualMachine,
                                                            std::map<ExecutorKey, EndBatchExecutionOpinion>&& otherSuccessfulExecutorEndBatchInfos,
                                                            std::map<ExecutorKey, EndBatchExecutionOpinion>&& otherUnsuccessfulExecutorEndBatchInfos,
                                                            std::optional<PublishedEndBatchExecutionTransactionInfo>&& publishedEndBatchInfo ) {
    return std::make_unique<BatchExecutionTask>( std::move( batch ), taskContext, virtualMachine,
                                                 std::move( otherSuccessfulExecutorEndBatchInfos ),
                                                 std::move( otherUnsuccessfulExecutorEndBatchInfos ),
                                                 std::move( publishedEndBatchInfo ));
}

}