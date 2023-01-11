/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BatchExecutionTask.h"
#include "ProofOfExecution.h"
#include "CallExecutionEnvironment.h"
#include "utils/Serializer.h"
#include "Messages.h"
#include "CallExecutionManager.h"
#include <magic_enum.hpp>
#include <storage/StorageErrorCode.h>
#include <virtualMachine/ExecutionErrorConidition.h>

namespace sirius::contract {

BatchExecutionTask::BatchExecutionTask(Batch&& batch,
                                       ContractEnvironment& contractEnvironment,
                                       ExecutorEnvironment& executorEnvironment,
                                       std::map<ExecutorKey, SuccessfulEndBatchExecutionOpinion>&& otherSuccessfulExecutorEndBatchOpinions,
                                       std::map<ExecutorKey, UnsuccessfulEndBatchExecutionOpinion>&& otherUnsuccessfulExecutorEndBatchOpinions,
                                       std::optional<PublishedEndBatchExecutionTransactionInfo>&& publishedEndBatchInfo)
        : BaseContractTask(executorEnvironment, contractEnvironment)
          , m_batch(std::move(batch))
          , m_callIterator(m_batch.m_callRequests.begin())
          , m_publishedEndBatchInfo(std::move(publishedEndBatchInfo))
          , m_otherSuccessfulExecutorEndBatchOpinions(
                std::move(otherSuccessfulExecutorEndBatchOpinions))
          , m_otherUnsuccessfulExecutorEndBatchOpinions(
                std::move(otherUnsuccessfulExecutorEndBatchOpinions)) {}

void BatchExecutionTask::run() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto[query, callback] = createAsyncQuery<void>([this](auto&& res) {
        if (!res) {
            const auto& ec = res.error();
            ASSERT(ec == storage::StorageError::storage_unavailable, m_executorEnvironment.logger())
            onUnableToExecuteBatch();
            return;
        }

        onInitiatedStorageModifications();
    }, [] {}, m_executorEnvironment, true, true);

    m_storageQuery = std::move(query);

    auto storage = m_executorEnvironment.storageModifier().lock();

    if (!storage) {
        onUnableToExecuteBatch();
        return;
    }

    storage->initiateModifications(m_contractEnvironment.driveKey(),
                                   storageModificationId(m_batch.m_batchIndex),
                                   callback);
}

void BatchExecutionTask::terminate() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_callExecutionManager.reset();

    m_storageQuery.reset();

    m_unsuccessfulExecutionTimer.cancel();
    m_successfulApprovalExpectationTimer.cancel();
    m_unsuccessfulApprovalExpectationTimer.cancel();
    m_unableToExecuteBatchTimer.cancel();
    m_shareOpinionTimer.cancel();

    m_finished = true;

    m_contractEnvironment.finishTask();
}

void BatchExecutionTask::onSuperContractCallExecuted(const CallId& callId, bool isManual, vm::CallExecutionResult&& result) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_callExecutionManager, m_executorEnvironment.logger())

    bool success = result.m_success;

    auto[query, callback] = createAsyncQuery<storage::SandboxModificationDigest>(
            [this, callId, isManual, result = std::move(result)](auto&& digest) mutable {

                if (!digest) {
                    const auto& ec = digest.error();
                    ASSERT(ec == storage::StorageError::storage_unavailable, m_executorEnvironment.logger())
                    onUnableToExecuteBatch();
                    return;
                }

                onAppliedSandboxStorageModifications(callId, isManual, std::forward<vm::CallExecutionResult>(result),
                                                     std::move(*digest));
            }, [] {}, m_executorEnvironment, true, true);

    m_storageQuery = std::move(query);

    auto storage = m_executorEnvironment.storageModifier().lock();

    if (!storage) {
        onUnableToExecuteBatch();
        return;
    }

    storage->applySandboxStorageModifications(m_contractEnvironment.driveKey(),
                                              success,
                                              callback);
}

// region message event handler

bool BatchExecutionTask::onEndBatchExecutionOpinionReceived(const SuccessfulEndBatchExecutionOpinion& opinion) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_finished) {
        return false;
    }

    if (opinion.m_batchIndex != m_batch.m_batchIndex) {
        return false;
    }

    if (!m_successfulEndBatchOpinion || validateOtherBatchInfo(opinion)) {
        m_otherSuccessfulExecutorEndBatchOpinions[opinion.m_executorKey] = opinion;
    }

    checkEndBatchTransactionReadiness();

    return true;
}

bool BatchExecutionTask::onEndBatchExecutionOpinionReceived(const UnsuccessfulEndBatchExecutionOpinion& opinion) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_finished) {
        return false;
    }

    if (opinion.m_batchIndex != m_batch.m_batchIndex) {
        return false;
    }

    if (!m_successfulEndBatchOpinion || validateOtherBatchInfo(opinion)) {
        m_otherUnsuccessfulExecutorEndBatchOpinions[opinion.m_executorKey] = opinion;
    }

    checkEndBatchTransactionReadiness();

    return true;
}

// endregion

void BatchExecutionTask::onInitiatedStorageModifications() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_storageQuery, m_executorEnvironment.logger())

    m_storageQuery.reset();

    executeNextCall();
}

void BatchExecutionTask::onInitiatedSandboxModification(vm::CallRequest&& callRequest) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_storageQuery, m_executorEnvironment.logger())

    m_storageQuery.reset();

    ASSERT(!m_callExecutionManager, m_executorEnvironment.logger())

    callRequest.m_proofOfExecutionPrefix = m_proofOfExecutionSecretData;

    auto callEnvironment = std::make_shared<CallExecutionEnvironment>(
            callRequest,
            m_executorEnvironment,
            m_contractEnvironment);

    bool isManual = callRequest.m_callLevel == vm::CallRequest::CallLevel::MANUAL;

    auto[query, callback] = createAsyncQuery<vm::CallExecutionResult>(
            [this, callId = callRequest.m_callId, isManual=isManual](auto&& res) {

                if (!res) {
                    const auto& ec = res.error();
                    ASSERT(ec == vm::ExecutionError::virtual_machine_unavailable ||
                           ec == vm::ExecutionError::storage_unavailable, m_executorEnvironment.logger())
                    onUnableToExecuteBatch();
                    return;
                }

                onSuperContractCallExecuted(callId, isManual, std::move(*res));
            }, [] {}, m_executorEnvironment, true, true);

    m_callExecutionManager = std::make_unique<CallExecutionManager>(m_executorEnvironment,
                                                                    callEnvironment,
                                                                    callEnvironment,
                                                                    callEnvironment,
                                                                    std::move(query));

    m_callExecutionManager->run(callRequest, std::move(callback));
}

void BatchExecutionTask::onAppliedSandboxStorageModifications(const CallId& callId,
                                                              bool manual,
                                                              vm::CallExecutionResult&& executionResult,
                                                              storage::SandboxModificationDigest&& digest) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_storageQuery, m_executorEnvironment.logger())

    ASSERT(m_callExecutionManager, m_executorEnvironment.logger())

    m_storageQuery.reset();

    if (!executionResult.m_success) {
        ASSERT(digest.m_success, m_executorEnvironment.logger())
    }

    m_proofOfExecutionSecretData = executionResult.m_proofOfExecutionSecretData;

    uint16_t status = 0;
    if (!digest.m_success) {
        // TOTO add enum
        status = 1;
    }

    auto blockchainHandler = m_callExecutionManager->blockchainQueryHandler();

    m_callsExecutionOpinions.push_back(SuccessfulCallExecutionOpinion{
            callId,
            manual,
            status,
            blockchainHandler->releasedTransactionHash(),
            CallExecutorParticipation{
                    executionResult.m_scConsumed,
                    executionResult.m_smConsumed
            }
    });

    m_callExecutionManager.reset();

    executeNextCall();
}

void BatchExecutionTask::onStorageHashEvaluated(storage::StorageState&& storageState) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_storageQuery, m_executorEnvironment.logger())

    m_storageQuery.reset();

    formSuccessfulEndBatchOpinion(storageState.m_storageHash,
                                  storageState.m_usedDriveSize,
                                  storageState.m_metaFilesSize,
                                  storageState.m_fileStructureSize);
}

// region blockchain event handler

bool BatchExecutionTask::onEndBatchExecutionPublished(const PublishedEndBatchExecutionTransactionInfo& info) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_finished) {
        return false;
    }

    if (info.m_batchIndex != m_batch.m_batchIndex) {
        return false;
    }

    m_publishedEndBatchInfo = info;

    if (m_successfulEndBatchOpinion) {
        processPublishedEndBatch();
    } else {
        // We are not able to process the transaction yet, we will do it as soon as the batch will be executed
    }

    return true;
}

bool BatchExecutionTask::onEndBatchExecutionFailed(const FailedEndBatchExecutionTransactionInfo& info) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_finished) {
        return false;
    }

    if (m_batch.m_batchIndex != info.m_batchIndex) {
        return false;
    }

    if (!m_publishedEndBatchInfo) {

        if (info.m_batchSuccess) {

            std::erase_if(m_otherSuccessfulExecutorEndBatchOpinions, [this](const auto& item) {
                return !validateOtherBatchInfo(item.second);
            });

            m_successfulEndBatchSent = false;

        } else {

            std::erase_if(m_otherUnsuccessfulExecutorEndBatchOpinions, [this](const auto& item) {
                return !validateOtherBatchInfo(item.second);
            });

            m_unsuccessfulEndBatchSent = false;

        }

        checkEndBatchTransactionReadiness();

    }

    return true;
}

// endregion

void BatchExecutionTask::formSuccessfulEndBatchOpinion(const StorageHash& storageHash,
                                                       uint64_t usedDriveSize, uint64_t metaFilesSize,
                                                       uint64_t fileStructureSize) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())
    m_successfulEndBatchOpinion = SuccessfulEndBatchExecutionOpinion();
    m_successfulEndBatchOpinion->m_contractKey = m_contractEnvironment.contractKey();
    m_successfulEndBatchOpinion->m_batchIndex = m_batch.m_batchIndex;

    auto verificationInformation = m_contractEnvironment.proofOfExecution().addToProof(m_proofOfExecutionSecretData);

    m_successfulEndBatchOpinion->m_proof = m_contractEnvironment.proofOfExecution().buildProof();

    m_successfulEndBatchOpinion->m_successfulBatchInfo = SuccessfulBatchInfo{storageHash, usedDriveSize,
                                                                             metaFilesSize,
                                                                             verificationInformation};

    m_successfulEndBatchOpinion->m_callsExecutionInfo = std::move(m_callsExecutionOpinions);

    m_successfulEndBatchOpinion->m_executorKey = m_executorEnvironment.keyPair().publicKey();

    m_successfulEndBatchOpinion->sign(m_executorEnvironment.keyPair());

    if (m_publishedEndBatchInfo) {
        processPublishedEndBatch();
        return;
    }

    shareOpinions();

    m_unsuccessfulExecutionTimer = m_executorEnvironment.threadManager().startTimer(
            m_contractEnvironment.contractConfig().unsuccessfulApprovalDelayMs(), [this] {
                onUnsuccessfulExecutionTimerExpiration();
            });

    std::erase_if(m_otherSuccessfulExecutorEndBatchOpinions, [this](const auto& item) {
        return !validateOtherBatchInfo(item.second);
    });

    std::erase_if(m_otherUnsuccessfulExecutorEndBatchOpinions, [this](const auto& item) {
        return !validateOtherBatchInfo(item.second);
    });

    checkEndBatchTransactionReadiness();
}

void BatchExecutionTask::processPublishedEndBatch() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_publishedEndBatchInfo, m_executorEnvironment.logger())
    ASSERT(m_successfulEndBatchOpinion, m_executorEnvironment.logger())

    bool batchIsSuccessful = m_publishedEndBatchInfo->m_batchSuccess;

    if (!batchIsSuccessful) {
        m_contractEnvironment.proofOfExecution().popFromProof();
    }

    if (batchIsSuccessful && (m_publishedEndBatchInfo->m_driveState !=
                              m_successfulEndBatchOpinion->m_successfulBatchInfo.m_storageHash ||
                              m_publishedEndBatchInfo->m_PoExVerificationInfo !=
                              m_successfulEndBatchOpinion->m_successfulBatchInfo.m_PoExVerificationInfo)) {
        // The received result differs from the common one, synchronization is needed

        m_executorEnvironment.logger().warn("Calculated storage hash differs from the published one");

        m_contractEnvironment.addSynchronizationTask();

        m_finished = true;

        m_contractEnvironment.finishTask();

    } else {

        auto storage = m_executorEnvironment.storageModifier().lock();

        if (!storage) {
            onUnableToExecuteBatch();
            return;
        }

        auto[query, callback] = createAsyncQuery<void>([this](auto&& res) {
            if (!res) {
                const auto& ec = res.error();
                ASSERT(ec == storage::StorageError::storage_unavailable, m_executorEnvironment.logger())
                onUnableToExecuteBatch();
                return;
            }

            onAppliedStorageModifications();

        }, [] {}, m_executorEnvironment, true, true);

        m_storageQuery = std::move(query);

        storage->applyStorageModifications(m_contractEnvironment.driveKey(),
                                           batchIsSuccessful,
                                           callback);
    }
}

bool BatchExecutionTask::validateOtherBatchInfo(const SuccessfulEndBatchExecutionOpinion& other) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_successfulEndBatchOpinion, m_executorEnvironment.logger())

    const auto& otherSuccessfulBatchInfo = other.m_successfulBatchInfo;
    const auto& successfulBatchInfo = m_successfulEndBatchOpinion->m_successfulBatchInfo;

    if (otherSuccessfulBatchInfo.m_PoExVerificationInfo != successfulBatchInfo.m_PoExVerificationInfo) {
        return false;
    }

    if (otherSuccessfulBatchInfo.m_storageHash != successfulBatchInfo.m_storageHash) {
        return false;
    }

    if (otherSuccessfulBatchInfo.m_usedStorageSize != successfulBatchInfo.m_usedStorageSize) {
        return false;
    }

    if (otherSuccessfulBatchInfo.m_metaFilesSize != successfulBatchInfo.m_metaFilesSize) {
        return false;
    }

    if (other.m_callsExecutionInfo.size() != m_successfulEndBatchOpinion->m_callsExecutionInfo.size()) {
        return false;
    }

    auto otherCallIt = other.m_callsExecutionInfo.begin();
    auto callIt = m_successfulEndBatchOpinion->m_callsExecutionInfo.begin();
    for (; otherCallIt != other.m_callsExecutionInfo.end(); otherCallIt++, callIt++) {
        if (otherCallIt->m_callId != callIt->m_callId) {
            return false;
        }

        if (otherCallIt->m_manual !=
            callIt->m_manual) {
            return false;
        }

        if (otherCallIt->m_callExecutionStatus != callIt->m_callExecutionStatus) {
            return false;
        }

        if (otherCallIt->m_releasedTransaction !=
            callIt->m_releasedTransaction) {
            return false;
        }
    }

    return true;
}

bool BatchExecutionTask::validateOtherBatchInfo(const UnsuccessfulEndBatchExecutionOpinion& other) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_successfulEndBatchOpinion, m_executorEnvironment.logger())

    if (other.m_callsExecutionInfo.size() != m_successfulEndBatchOpinion->m_callsExecutionInfo.size()) {
        return false;
    }

    auto otherCallIt = other.m_callsExecutionInfo.begin();
    auto callIt = m_successfulEndBatchOpinion->m_callsExecutionInfo.begin();
    for (; otherCallIt != other.m_callsExecutionInfo.end(); otherCallIt++, callIt++) {
        if (otherCallIt->m_callId != callIt->m_callId) {
            return false;
        }
    }

    return true;
}

void BatchExecutionTask::checkEndBatchTransactionReadiness() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_successfulEndBatchOpinion &&
        2 * m_otherSuccessfulExecutorEndBatchOpinions.size() >=
        3 * (m_contractEnvironment.executors().size() + 1)) {
        // Enough signatures for successful batch

        if (!m_successfulEndBatchSent) {

            m_successfulEndBatchSent = true;

            auto tx = createMultisigTransactionInfo(*m_successfulEndBatchOpinion,
                                                    std::move(m_otherSuccessfulExecutorEndBatchOpinions));

            m_successfulApprovalExpectationTimer = m_executorEnvironment.threadManager().startTimer(
                    m_executorEnvironment.executorConfig().successfulExecutionDelayMs(),
                    [this, tx = std::move(tx)] {
                        sendEndBatchTransaction(tx);
                    });

        }
    } else if (m_unsuccessfulEndBatchOpinion &&
               2 * m_otherUnsuccessfulExecutorEndBatchOpinions.size() >=
               3 * (m_contractEnvironment.executors().size() + 1)) {

        if (!m_unsuccessfulEndBatchSent) {

            m_unsuccessfulEndBatchSent = true;

            auto tx = createMultisigTransactionInfo(*m_unsuccessfulEndBatchOpinion,
                                                    std::move(m_otherUnsuccessfulExecutorEndBatchOpinions));
            m_unsuccessfulApprovalExpectationTimer = m_executorEnvironment.threadManager().startTimer(
                    m_executorEnvironment.executorConfig().unsuccessfulExecutionDelayMs(),
                    [this, tx = std::move(tx)] {
                        sendEndBatchTransaction(tx);
                    });
        }
    }
}

SuccessfulEndBatchExecutionTransactionInfo
BatchExecutionTask::createMultisigTransactionInfo(const SuccessfulEndBatchExecutionOpinion& transactionOpinion,
                                                  std::map<ExecutorKey, SuccessfulEndBatchExecutionOpinion>&& otherTransactionOpinions) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    SuccessfulEndBatchExecutionTransactionInfo multisigTransactionInfo;

    // Fill common information
    multisigTransactionInfo.m_contractKey = transactionOpinion.m_contractKey;
    multisigTransactionInfo.m_batchIndex = transactionOpinion.m_batchIndex;
    multisigTransactionInfo.m_successfulBatchInfo = transactionOpinion.m_successfulBatchInfo;

    for (const auto& call: transactionOpinion.m_callsExecutionInfo) {
        SuccessfulCallExecutionInfo callInfo;
        callInfo.m_callId = call.m_callId;
        callInfo.m_manual = call.m_manual;
        callInfo.m_callExecutionStatus = call.m_callExecutionStatus;
        callInfo.m_releasedTransaction = call.m_releasedTransaction;
        multisigTransactionInfo.m_callsExecutionInfo.push_back(callInfo);
    }

    otherTransactionOpinions[m_executorEnvironment.keyPair().publicKey()] = transactionOpinion;

    for (const auto&[_, otherOpinion]: otherTransactionOpinions) {
        multisigTransactionInfo.m_executorKeys.push_back(otherOpinion.m_executorKey);
        multisigTransactionInfo.m_signatures.push_back(otherOpinion.m_signature);
        multisigTransactionInfo.m_proofs.push_back(otherOpinion.m_proof);

        ASSERT(multisigTransactionInfo.m_callsExecutionInfo.size() == otherOpinion.m_callsExecutionInfo.size(),
               m_executorEnvironment.logger())
        auto txCallIt = multisigTransactionInfo.m_callsExecutionInfo.begin();
        auto otherCallIt = otherOpinion.m_callsExecutionInfo.begin();
        for (; txCallIt != multisigTransactionInfo.m_callsExecutionInfo.end(); txCallIt++, otherCallIt++) {
            ASSERT(txCallIt->m_callId == otherCallIt->m_callId, m_executorEnvironment.logger())
            txCallIt->m_executorsParticipation.push_back(otherCallIt->m_executorParticipation);
        }
    }

    return multisigTransactionInfo;
}

UnsuccessfulEndBatchExecutionTransactionInfo
BatchExecutionTask::createMultisigTransactionInfo(const UnsuccessfulEndBatchExecutionOpinion& transactionOpinion,
                                                  std::map<ExecutorKey, UnsuccessfulEndBatchExecutionOpinion>&& otherTransactionOpinions) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    UnsuccessfulEndBatchExecutionTransactionInfo multisigTransactionInfo;

    // Fill common information
    multisigTransactionInfo.m_contractKey = transactionOpinion.m_contractKey;
    multisigTransactionInfo.m_batchIndex = transactionOpinion.m_batchIndex;

    for (const auto& call: transactionOpinion.m_callsExecutionInfo) {
        UnsuccessfulCallExecutionInfo callInfo;
        callInfo.m_callId = call.m_callId;
        multisigTransactionInfo.m_callsExecutionInfo.push_back(callInfo);
    }

    otherTransactionOpinions[m_executorEnvironment.keyPair().publicKey()] = transactionOpinion;

    for (const auto&[_, otherOpinion]: otherTransactionOpinions) {
        multisigTransactionInfo.m_executorKeys.push_back(otherOpinion.m_executorKey);
        multisigTransactionInfo.m_signatures.push_back(otherOpinion.m_signature);
        multisigTransactionInfo.m_proofs.push_back(otherOpinion.m_proof);
        ASSERT(multisigTransactionInfo.m_callsExecutionInfo.size() == otherOpinion.m_callsExecutionInfo.size(),
               m_executorEnvironment.logger())
        auto txCallIt = multisigTransactionInfo.m_callsExecutionInfo.begin();
        auto otherCallIt = otherOpinion.m_callsExecutionInfo.begin();
        for (; txCallIt != multisigTransactionInfo.m_callsExecutionInfo.end(); txCallIt++, otherCallIt++) {
            ASSERT(txCallIt->m_callId == otherCallIt->m_callId, m_executorEnvironment.logger())
            txCallIt->m_executorsParticipation.push_back(otherCallIt->m_executorParticipation);
        }
    }

    return multisigTransactionInfo;
}

void BatchExecutionTask::sendEndBatchTransaction(const SuccessfulEndBatchExecutionTransactionInfo& transactionInfo) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.executorEventHandler().endBatchTransactionIsReady(transactionInfo);
}

void BatchExecutionTask::sendEndBatchTransaction(const UnsuccessfulEndBatchExecutionTransactionInfo& transactionInfo) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.executorEventHandler().endBatchTransactionIsReady(transactionInfo);
}

void BatchExecutionTask::executeNextCall() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_callExecutionManager, m_executorEnvironment.logger())

    ASSERT(!m_storageQuery, m_executorEnvironment.logger())

    if (m_callIterator != m_batch.m_callRequests.end()) {
        auto callRequest = *m_callIterator;
        m_callIterator++;

        auto storage = m_executorEnvironment.storageModifier().lock();

        if (!storage) {
            onUnableToExecuteBatch();
            return;
        }

        auto[query, callback] = createAsyncQuery<void>([=, this](auto&& res) mutable {
            if (!res) {
                const auto& ec = res.error();
                ASSERT(ec == storage::StorageError::storage_unavailable, m_executorEnvironment.logger())
                onUnableToExecuteBatch();
                return;
            }

            onInitiatedSandboxModification(std::move(callRequest));
        }, [] {}, m_executorEnvironment, true, true);

        m_storageQuery = std::move(query);

        storage->initiateSandboxModifications(m_contractEnvironment.driveKey(),
                                              callback);
    } else {

        auto storage = m_executorEnvironment.storageModifier().lock();

        if (!storage) {
            onUnableToExecuteBatch();
            return;
        }

        computeProofOfExecution();

        auto[query, callback] = createAsyncQuery<storage::StorageState>([this](auto&& state) {

            if (!state) {
                const auto& ec = state.error();
                ASSERT(ec == storage::StorageError::storage_unavailable, m_executorEnvironment.logger())
                onUnableToExecuteBatch();
                return;
            }

            onStorageHashEvaluated(std::move(*state));
        }, [] {}, m_executorEnvironment, true, true);

        m_storageQuery = std::move(query);

        storage->evaluateStorageHash(
                m_contractEnvironment.driveKey(), callback);
    }
}

void BatchExecutionTask::onUnsuccessfulExecutionTimerExpiration() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_successfulEndBatchOpinion, m_executorEnvironment.logger())
    ASSERT(!m_unsuccessfulEndBatchOpinion, m_executorEnvironment.logger())

    m_unsuccessfulEndBatchOpinion = UnsuccessfulEndBatchExecutionOpinion();
    m_unsuccessfulEndBatchOpinion->m_contractKey = m_successfulEndBatchOpinion->m_contractKey;
    m_unsuccessfulEndBatchOpinion->m_batchIndex = m_successfulEndBatchOpinion->m_batchIndex;
    m_unsuccessfulEndBatchOpinion->m_proof = m_successfulEndBatchOpinion->m_proof;
    m_unsuccessfulEndBatchOpinion->m_executorKey = m_successfulEndBatchOpinion->m_executorKey;



    m_unsuccessfulEndBatchOpinion->m_callsExecutionInfo.reserve(m_successfulEndBatchOpinion->m_callsExecutionInfo.size());
    for (const auto& successfulCall: m_successfulEndBatchOpinion->m_callsExecutionInfo) {
        UnsuccessfulCallExecutionOpinion unsuccessfulCall;
        unsuccessfulCall.m_callId = successfulCall.m_callId;
        unsuccessfulCall.m_manual = successfulCall.m_manual;
        unsuccessfulCall.m_executorParticipation = successfulCall.m_executorParticipation;
    }

    m_unsuccessfulEndBatchOpinion->sign(m_executorEnvironment.keyPair());

    // The opinion will be shared when the share opinion timer ticks
}

void BatchExecutionTask::computeProofOfExecution() {

}

void BatchExecutionTask::onUnableToExecuteBatch() {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_contractEnvironment.delayBatchExecution(m_batch);

    m_finished = true;

    m_unableToExecuteBatchTimer = Timer(m_executorEnvironment.threadManager().context(),
                                        m_executorEnvironment.executorConfig().serviceUnavailableTimeoutMs(), [this] {
                m_contractEnvironment.finishTask();
            });
}

void BatchExecutionTask::onAppliedStorageModifications() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    const auto& cosigners = m_publishedEndBatchInfo->m_cosigners;
    if (std::find(cosigners.begin(), cosigners.end(), m_executorEnvironment.keyPair().publicKey()) ==
        cosigners.end()) {
        EndBatchExecutionSingleTransactionInfo singleTx = {m_contractEnvironment.contractKey(),
                                                           m_batch.m_batchIndex,
                                                           m_contractEnvironment.proofOfExecution().buildProof()};
        m_executorEnvironment.executorEventHandler().endBatchSingleTransactionIsReady(singleTx);
    }

    m_finished = true;

    m_contractEnvironment.finishTask();
}

void BatchExecutionTask::shareOpinions() {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto messenger = m_executorEnvironment.messenger().lock();

    if (messenger) {

        ASSERT(m_successfulEndBatchOpinion, m_executorEnvironment.logger())

        for (const auto& [executor, _]: m_contractEnvironment.executors()) {
            auto serializedInfo = utils::serialize(*m_successfulEndBatchOpinion);
            auto tag = magic_enum::enum_name(MessageTag::SUCCESSFUL_END_BATCH);
            messenger->sendMessage(messenger::OutputMessage{executor, {tag.begin(), tag.end()}, serializedInfo});
        }

        if (m_unsuccessfulEndBatchOpinion) {
            for (const auto& [executor, _]: m_contractEnvironment.executors()) {
                auto serializedInfo = utils::serialize(*m_unsuccessfulEndBatchOpinion);
                auto tag = magic_enum::enum_name(MessageTag::UNSUCCESSFUL_END_BATCH);
                messenger->sendMessage(messenger::OutputMessage{executor, {tag.begin(), tag.end()}, serializedInfo});
            }
        }
    } else {
        m_executorEnvironment.logger().warn("Messenger not available");
    }

    m_shareOpinionTimer = Timer(m_executorEnvironment.threadManager().context(),
                                m_executorEnvironment.executorConfig().shareOpinionTimeoutMs(), [this] {
                shareOpinions();
            });
}

}