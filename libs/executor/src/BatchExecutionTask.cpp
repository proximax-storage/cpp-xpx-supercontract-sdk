/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BatchExecutionTask.h"
#include "ProofOfExecution.h"
#include "ManualCallBlockchainQueryHandler.h"
#include "InternetQueryHandler.h"
#include "StorageQueryHandler.h"
#include "utils/Serializer.h"
#include "Messages.h"
#include "CallExecutionManager.h"
#include <magic_enum.hpp>
#include <storage/StorageErrorCode.h>
#include <virtualMachine/ExecutionErrorConidition.h>
#include <storage/StorageErrorCode.h>
#include <utils/IntegerMath.h>
#include <blockchain/TransactionBuilder.h>

namespace sirius::contract {

namespace {

bool enoughOpinions(uint64_t opinionsReceived, uint64_t totalExecutors) {
    return 3 * opinionsReceived > 2 * totalExecutors;
}

}

BatchExecutionTask::BatchExecutionTask(Batch&& batch,
                                       std::shared_ptr<AsyncQueryCallback<void>>&& onTaskFinishedCallback,
                                       ContractEnvironment& contractEnvironment,
                                       ExecutorEnvironment& executorEnvironment)
        : BaseContractTask(executorEnvironment, contractEnvironment)
          , m_batch(std::move(batch))
          , m_callIterator(m_batch.m_callRequests.begin())
          , m_onTaskFinishedCallback(std::move(onTaskFinishedCallback)) {}

void BatchExecutionTask::run() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Batch execution task is run. "
                                        "Contract key: {}, "
                                        "batch index: {}, "
                                        "number of calls in the batch: {}",
                                        m_contractEnvironment.contractKey(),
                                        m_batch.m_batchIndex,
                                        m_batch.m_callRequests.size());

    auto[query, callback] = createAsyncQuery<std::unique_ptr<storage::StorageModification>>([this](auto&& res) {
        if (!res) {
            const auto& ec = res.error();
            ASSERT(ec == storage::StorageError::storage_unavailable, m_executorEnvironment.logger())
            onUnableToExecuteBatch(ec);
            return;
        }

        onInitiatedStorageModifications(std::move(*res));
    }, [] {}, m_executorEnvironment, true, true);

    m_storageQuery = std::move(query);

    auto storage = m_executorEnvironment.storage().lock();

    if (!storage) {
        onUnableToExecuteBatch(storage::make_error_code(storage::StorageError::storage_unavailable));
        return;
    }

    storage->initiateModifications(m_contractEnvironment.driveKey(),
                                   storageModificationId(m_batch.m_batchIndex),
                                   callback);
}

void BatchExecutionTask::terminate() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.logger().warn("Batch execution task is terminated. "
                                        "Contract key: {}, batch index: {}",
                                        m_contractEnvironment.contractKey(),
                                        m_batch.m_batchIndex);

    m_callExecutionManager.reset();

    m_storageQuery.reset();

    m_unsuccessfulExecutionTimer.cancel();
    m_successfulApprovalExpectationTimer.cancel();
    m_unsuccessfulApprovalExpectationTimer.cancel();
    m_unableToExecuteBatchTimer.cancel();
    m_shareOpinionTimer.cancel();

    m_finished = true;

    m_onTaskFinishedCallback->postReply(expected<void>());
}

void BatchExecutionTask::onSuperContractCallExecuted(std::shared_ptr<CallRequest>&& callRequest,
                                                     vm::CallExecutionResult&& result) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_callExecutionManager, m_executorEnvironment.logger())

    bool success = result.m_success;

    auto[query, callback] = createAsyncQuery<storage::SandboxModificationDigest>(
            [this, callRequest = std::move(callRequest), result = std::move(result)](auto&& digest) mutable {

                if (!digest) {
                    const auto& ec = digest.error();
                    ASSERT(ec == storage::StorageError::storage_unavailable, m_executorEnvironment.logger())
                    onUnableToExecuteBatch(ec);
                    return;
                }

                onAppliedSandboxStorageModifications(std::move(callRequest),
                                                     std::forward<vm::CallExecutionResult>(result),
                                                     std::move(*digest));
            }, [] {}, m_executorEnvironment, true, true);

    m_storageQuery = std::move(query);

    m_sandboxModification->applySandboxModification(success, callback);
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

void BatchExecutionTask::onInitiatedStorageModifications(std::unique_ptr<storage::StorageModification>&& storageModification) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_storageQuery, m_executorEnvironment.logger())

    m_storageQuery.reset();

    m_storageModification = std::move(storageModification);

    executeNextCall();
}

void BatchExecutionTask::onInitiatedSandboxModification(std::unique_ptr<storage::SandboxModification>&& sandboxModification,
                                                        std::shared_ptr<CallRequest>&& callRequest) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_storageQuery, m_executorEnvironment.logger())

    m_storageQuery.reset();

    m_sandboxModification = std::move(sandboxModification);

    ASSERT(!m_callExecutionManager, m_executorEnvironment.logger())

    bool isManual = callRequest->isManual();

    vm::CallRequest vmCallRequest(callRequest->callId(),
                                  callRequest->file(),
                                  callRequest->function(),
                                  callRequest->arguments(),
                                  callRequest->executionPayment() *
                                  m_executorEnvironment.executorConfig().executionPaymentToGasMultiplier(),
                                  callRequest->downloadPayment() *
                                  m_executorEnvironment.executorConfig().downloadPaymentToGasMultiplier(),
                                  isManual ? vm::CallRequest::CallLevel::MANUAL : vm::CallRequest::CallLevel::AUTOMATIC,
                                  m_proofOfExecutionSecretData,
								  m_contractEnvironment.driveKey());

    auto[query, callback] = createAsyncQuery<vm::CallExecutionResult>(
            [this, callRequest](auto&& res) mutable {

                if (!res) {
                    const auto& ec = res.error();
                    ASSERT(ec == vm::ExecutionError::virtual_machine_unavailable ||
                           ec == vm::ExecutionError::storage_unavailable, m_executorEnvironment.logger())
                    onUnableToExecuteBatch(ec);
                    return;
                }

                onSuperContractCallExecuted(std::move(callRequest), std::move(*res));
            }, [] {}, m_executorEnvironment, true, true);

    std::shared_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler;
    if (isManual) {
        blockchainQueryHandler = std::make_shared<ManualCallBlockchainQueryHandler>(
                m_executorEnvironment,
                m_contractEnvironment,
                callRequest->callerKey(),
                callRequest->blockHeight(),
                callRequest->executionPayment(),
                callRequest->downloadPayment(),
                callRequest->callId().array(),
                callRequest->servicePayments());
    } else {
        blockchainQueryHandler = std::make_shared<AutomaticExecutionsBlockchainQueryHandler>(
                m_executorEnvironment,
                m_contractEnvironment,
                callRequest->callerKey(),
                callRequest->blockHeight(),
                callRequest->executionPayment(),
                callRequest->downloadPayment());
    }

    m_callExecutionManager = std::make_unique<CallExecutionManager>(
            m_executorEnvironment,
            std::make_shared<InternetQueryHandler>(callRequest->callId(),
                                                   m_executorEnvironment.executorConfig().maxInternetConnections(),
                                                   m_executorEnvironment,
                                                   m_contractEnvironment),
            blockchainQueryHandler,
            std::make_shared<StorageQueryHandler>(callRequest->callId(),
                                                  m_executorEnvironment,
                                                  m_contractEnvironment,
                                                  m_sandboxModification,
                                                  m_executorEnvironment.executorConfig().storagePathPrefix()),
            std::move(query),
            vmCallRequest,
            std::move(callback));

    m_callExecutionManager->run();
}

void BatchExecutionTask::onAppliedSandboxStorageModifications(std::shared_ptr<CallRequest>&& callRequest,
                                                              vm::CallExecutionResult&& executionResult,
                                                              storage::SandboxModificationDigest&& digest) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_storageQuery, m_executorEnvironment.logger())

    ASSERT(m_callExecutionManager, m_executorEnvironment.logger())

    m_storageQuery.reset();

    if (!executionResult.m_success) {
        ASSERT(!digest.m_success, m_executorEnvironment.logger())
    }

    m_proofOfExecutionSecretData = executionResult.m_proofOfExecutionSecretData;

    uint16_t status = 0;
    if (!digest.m_success) {
        // TOTO add enum
        status = 1;
    }

    auto blockchainHandler = m_callExecutionManager->blockchainQueryHandler();

    bool isManual = callRequest->isManual();

    auto actualExecutionPayment = utils::DivideCeil(executionResult.m_execution_gas_consumed,
                                                    m_executorEnvironment.executorConfig().executionPaymentToGasMultiplier());
    actualExecutionPayment = std::min(actualExecutionPayment, callRequest->executionPayment());

    auto actualDownloadPayment = utils::DivideCeil(executionResult.m_download_gas_consumed,
                                                   m_executorEnvironment.executorConfig().downloadPaymentToGasMultiplier());
    actualDownloadPayment = std::min(actualDownloadPayment, callRequest->downloadPayment());

    Hash256 releasedTransactionsHash;
    if (status == 0 && executionResult.m_transaction) {
        auto[hash, transaction] = blockchain::buildAggregatedTransaction(
                m_executorEnvironment.executorConfig().networkIdentifier(), m_contractEnvironment.contractKey(),
                *executionResult.m_transaction);
        releasedTransactionsHash = hash;
        m_releasedTransactions.emplace(hash, std::move(transaction));
    }

    m_callsExecutionOpinions.push_back(SuccessfulCallExecutionOpinion{
            callRequest->callId(),
            isManual,
            callRequest->blockHeight(),
            status,
            releasedTransactionsHash,
            blockchain::CallExecutorParticipation{
                    actualExecutionPayment,
                    actualDownloadPayment
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

bool BatchExecutionTask::onEndBatchExecutionPublished(const blockchain::PublishedEndBatchExecutionTransactionInfo& info) {

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

bool BatchExecutionTask::onEndBatchExecutionFailed(const blockchain::FailedEndBatchExecutionTransactionInfo& info) {

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
    m_successfulEndBatchOpinion->m_automaticExecutionsCheckedUpTo = m_batch.m_automaticExecutionsCheckedUpTo;

    auto verificationInformation = m_contractEnvironment.proofOfExecution().addToProof(m_proofOfExecutionSecretData);

    m_successfulEndBatchOpinion->m_proof = m_contractEnvironment.proofOfExecution().buildActualProof();

    m_successfulEndBatchOpinion->m_successfulBatchInfo = blockchain::SuccessfulBatchInfo{storageHash, usedDriveSize,
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

        m_executorEnvironment.logger().warn("Unsuccessful batch is published. "
                                            "Contract key: {}, batch index: {}",
                                            m_contractEnvironment.contractKey(),
                                            m_batch.m_batchIndex);

        m_contractEnvironment.proofOfExecution().popFromProof();
    }

    if (batchIsSuccessful && (m_publishedEndBatchInfo->m_driveState !=
                              m_successfulEndBatchOpinion->m_successfulBatchInfo.m_storageHash ||
                              m_publishedEndBatchInfo->m_PoExVerificationInfo !=
                              m_successfulEndBatchOpinion->m_successfulBatchInfo.m_PoExVerificationInfo)) {
        // The received result differs from the common one, synchronization is needed

        m_executorEnvironment.logger().warn("Published batch execution result differs from the published one. "
                                            "Contract key: {}, batch index: {}",
                                            m_contractEnvironment.contractKey(),
                                            m_batch.m_batchIndex);

        m_contractEnvironment.addSynchronizationTask();

        m_finished = true;

        m_onTaskFinishedCallback->postReply(expected<void>());

    } else {

        m_executorEnvironment.logger().info("Successful and correct batch execution is published. "
                                            "Contract key: {}, batch index: {}",
                                            m_contractEnvironment.contractKey(),
                                            m_batch.m_batchIndex);

        for (const auto&[hash, tx]: m_releasedTransactions) {
            m_executorEnvironment.executorEventHandler().releasedTransactionsAreReady(tx);
        }

        auto[query, callback] = createAsyncQuery<void>([this](auto&& res) {
            if (!res) {
                const auto& ec = res.error();
                ASSERT(ec == storage::StorageError::storage_unavailable, m_executorEnvironment.logger())
                onUnableToExecuteBatch(ec);
                return;
            }

            onAppliedStorageModifications();

        }, [] {}, m_executorEnvironment, true, true);

        m_storageQuery = std::move(query);

        m_storageModification->applyStorageModification(batchIsSuccessful, callback);
    }
}

bool BatchExecutionTask::validateOtherBatchInfo(const SuccessfulEndBatchExecutionOpinion& other) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_successfulEndBatchOpinion, m_executorEnvironment.logger())

    const auto& executors = m_contractEnvironment.executors();

    auto executorIt = executors.find(other.m_executorKey);

    if (executorIt == executors.end()) {
        m_executorEnvironment.logger().warn("Invalid successful opinion received: executor. "
                                            "Contract key: {}, batch index: {}",
                                            m_contractEnvironment.contractKey(),
                                            m_batch.m_batchIndex);
        return false;
    }

    const auto& otherSuccessfulBatchInfo = other.m_successfulBatchInfo;
    const auto& successfulBatchInfo = m_successfulEndBatchOpinion->m_successfulBatchInfo;

    if (other.m_automaticExecutionsCheckedUpTo != m_successfulEndBatchOpinion->m_automaticExecutionsCheckedUpTo) {
        m_executorEnvironment.logger().warn("Invalid successful opinion received: automatic executions checked up to. "
                                            "Contract key: {}, batch index: {}",
                                            m_contractEnvironment.contractKey(),
                                            m_batch.m_batchIndex);
        return false;
    }

    if (otherSuccessfulBatchInfo.m_PoExVerificationInfo != successfulBatchInfo.m_PoExVerificationInfo) {
        m_executorEnvironment.logger().warn("Invalid successful opinion received: PoEx verification info. "
                                            "Contract key: {}, batch index: {}",
                                            m_contractEnvironment.contractKey(),
                                            m_batch.m_batchIndex);
        return false;
    }

    if (otherSuccessfulBatchInfo.m_storageHash != successfulBatchInfo.m_storageHash) {
        m_executorEnvironment.logger().warn("Invalid successful opinion received: storage hash. "
                                            "Contract key: {}, batch index: {}",
                                            m_contractEnvironment.contractKey(),
                                            m_batch.m_batchIndex);
        return false;
    }

    if (otherSuccessfulBatchInfo.m_usedStorageSize != successfulBatchInfo.m_usedStorageSize) {
        m_executorEnvironment.logger().warn("Invalid successful opinion received: used storage size. "
                                            "Contract key: {}, batch index: {}",
                                            m_contractEnvironment.contractKey(),
                                            m_batch.m_batchIndex);
        return false;
    }

    if (otherSuccessfulBatchInfo.m_metaFilesSize != successfulBatchInfo.m_metaFilesSize) {
        m_executorEnvironment.logger().warn("Invalid successful opinion received: meta files size. "
                                            "Contract key: {}, batch index: {}",
                                            m_contractEnvironment.contractKey(),
                                            m_batch.m_batchIndex);
        return false;
    }

    if (other.m_callsExecutionInfo.size() != m_successfulEndBatchOpinion->m_callsExecutionInfo.size()) {
        m_executorEnvironment.logger().warn("Invalid successful opinion received: calls number. "
                                            "Contract key: {}, batch index: {}",
                                            m_contractEnvironment.contractKey(),
                                            m_batch.m_batchIndex);
        return false;
    }

    ASSERT(m_successfulEndBatchOpinion->m_callsExecutionInfo.size() == m_batch.m_callRequests.size(),
           m_executorEnvironment.logger())

    auto otherCallIt = other.m_callsExecutionInfo.cbegin();
    auto callIt = m_successfulEndBatchOpinion->m_callsExecutionInfo.cbegin();
    auto batchCallIt = m_batch.m_callRequests.cbegin();
    for (; otherCallIt != other.m_callsExecutionInfo.end(); otherCallIt++, callIt++, batchCallIt++) {
        if (otherCallIt->m_callId != callIt->m_callId) {
            m_executorEnvironment.logger().warn("Invalid successful opinion received: call id. "
                                                "Contract key: {}, batch index: {}",
                                                m_contractEnvironment.contractKey(),
                                                m_batch.m_batchIndex);
            return false;
        }

        if (otherCallIt->m_manual !=
            callIt->m_manual) {
            m_executorEnvironment.logger().warn("Invalid successful opinion received: call level. "
                                                "Contract key: {}, batch index: {}",
                                                m_contractEnvironment.contractKey(),
                                                m_batch.m_batchIndex);
            return false;
        }

        if (otherCallIt->m_callExecutionStatus != callIt->m_callExecutionStatus) {
            m_executorEnvironment.logger().warn("Invalid successful opinion received: call execution status. "
                                                "Contract key: {}, batch index: {}",
                                                m_contractEnvironment.contractKey(),
                                                m_batch.m_batchIndex);
            return false;
        }

        if (otherCallIt->m_releasedTransaction !=
            callIt->m_releasedTransaction) {
            m_executorEnvironment.logger().warn("Invalid successful opinion received: released transaction. "
                                                "Contract key: {}, batch index: {}",
                                                m_contractEnvironment.contractKey(),
                                                m_batch.m_batchIndex);
            return false;
        }

        if (otherCallIt->m_executorParticipation.m_scConsumed > (*batchCallIt)->executionPayment()) {
            m_executorEnvironment.logger().warn("Invalid successful opinion received: execution payment. "
                                                "Contract key: {}, batch index: {}",
                                                m_contractEnvironment.contractKey(),
                                                m_batch.m_batchIndex);
            return false;
        }

        if (otherCallIt->m_executorParticipation.m_smConsumed > (*batchCallIt)->downloadPayment()) {
            m_executorEnvironment.logger().warn("Invalid successful opinion received: download payment. "
                                                "Contract key: {}, batch index: {}",
                                                m_contractEnvironment.contractKey(),
                                                m_batch.m_batchIndex);
            return false;
        }
    }

    if (!m_contractEnvironment.proofOfExecution().verifyProof(executorIt->first, executorIt->second, other.m_proof,
                                                              other.m_batchIndex,
                                                              other.m_successfulBatchInfo.m_PoExVerificationInfo)) {
        m_executorEnvironment.logger().warn("Invalid successful opinion received: PoEx. "
                                            "Contract key: {}, batch index: {}",
                                            m_contractEnvironment.contractKey(),
                                            m_batch.m_batchIndex);
        return false;
    }

    return true;
}

bool BatchExecutionTask::validateOtherBatchInfo(const UnsuccessfulEndBatchExecutionOpinion& other) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_successfulEndBatchOpinion, m_executorEnvironment.logger())

    const auto& executors = m_contractEnvironment.executors();

    auto executorIt = executors.find(other.m_executorKey);

    if (executorIt == executors.end()) {
        m_executorEnvironment.logger().warn("Invalid unsuccessful opinion received: executor. "
                                            "Contract key: {}, batch index: {}",
                                            m_contractEnvironment.contractKey(),
                                            m_batch.m_batchIndex);
        return false;
    }

    if (other.m_automaticExecutionsCheckedUpTo != m_successfulEndBatchOpinion->m_automaticExecutionsCheckedUpTo) {
        m_executorEnvironment.logger().warn(
                "Invalid unsuccessful opinion received: automatic executions checked up to. "
                "Contract key: {}, batch index: {}",
                m_contractEnvironment.contractKey(),
                m_batch.m_batchIndex);
        return false;
    }

    if (other.m_callsExecutionInfo.size() != m_successfulEndBatchOpinion->m_callsExecutionInfo.size()) {
        m_executorEnvironment.logger().warn("Invalid unsuccessful opinion received: calls number. "
                                            "Contract key: {}, batch index: {}",
                                            m_contractEnvironment.contractKey(),
                                            m_batch.m_batchIndex);
        return false;
    }

    ASSERT(m_successfulEndBatchOpinion->m_callsExecutionInfo.size() == m_batch.m_callRequests.size(),
           m_executorEnvironment.logger())

    auto otherCallIt = other.m_callsExecutionInfo.cbegin();
    auto callIt = m_successfulEndBatchOpinion->m_callsExecutionInfo.cbegin();
    auto batchCallIt = m_batch.m_callRequests.cbegin();
    for (; otherCallIt != other.m_callsExecutionInfo.end(); otherCallIt++, callIt++, batchCallIt++) {
        if (otherCallIt->m_callId != callIt->m_callId) {
            m_executorEnvironment.logger().warn("Invalid unsuccessful opinion received: call id. "
                                                "Contract key: {}, batch index: {}",
                                                m_contractEnvironment.contractKey(),
                                                m_batch.m_batchIndex);
            return false;
        }

        if (otherCallIt->m_manual !=
            callIt->m_manual) {
            m_executorEnvironment.logger().warn("Invalid unsuccessful opinion received: call level. "
                                                "Contract key: {}, batch index: {}",
                                                m_contractEnvironment.contractKey(),
                                                m_batch.m_batchIndex);
            return false;
        }

        if (otherCallIt->m_executorParticipation.m_scConsumed > (*batchCallIt)->executionPayment()) {
            m_executorEnvironment.logger().warn("Invalid unsuccessful opinion received: execution payment. "
                                                "Contract key: {}, batch index: {}",
                                                m_contractEnvironment.contractKey(),
                                                m_batch.m_batchIndex);
            return false;
        }

        if (otherCallIt->m_executorParticipation.m_smConsumed > (*batchCallIt)->downloadPayment()) {
            m_executorEnvironment.logger().warn("Invalid unsuccessful opinion received: download payment. "
                                                "Contract key: {}, batch index: {}",
                                                m_contractEnvironment.contractKey(),
                                                m_batch.m_batchIndex);
            return false;
        }
    }

    if (!m_contractEnvironment.proofOfExecution().verifyProof(executorIt->first, executorIt->second, other.m_proof,
                                                              other.m_batchIndex,
                                                              crypto::CurvePoint())) {
        m_executorEnvironment.logger().warn("Invalid unsuccessful opinion received: PoEx. "
                                            "Contract key: {}, batch index: {}",
                                            m_contractEnvironment.contractKey(),
                                            m_batch.m_batchIndex);
        return false;
    }

    return true;
}

void BatchExecutionTask::checkEndBatchTransactionReadiness() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.logger().debug("Check opinions number. "
                                         "Contract key: {}, batch index: {}, total executors: {}."
                                         "Successful opinion: {}, other successful opinions: {}."
                                         "Unsuccessful opinion: {}, other unsuccessful opinions: {}",
                                         m_contractEnvironment.contractKey(),
                                         m_batch.m_batchIndex,
                                         m_contractEnvironment.executors().size() + 1,
                                         static_cast<bool>(m_successfulEndBatchOpinion),
                                         m_otherSuccessfulExecutorEndBatchOpinions.size(),
                                         static_cast<bool>(m_unsuccessfulEndBatchOpinion),
                                         m_otherUnsuccessfulExecutorEndBatchOpinions.size());

    if (m_successfulEndBatchOpinion &&
        enoughOpinions(m_otherSuccessfulExecutorEndBatchOpinions.size() + 1,
                       m_contractEnvironment.executors().size() + 1)) {
        // Enough signatures for successful batch

        if (!m_successfulEndBatchSent) {

            m_successfulEndBatchSent = true;

            m_successfulApprovalExpectationTimer = m_executorEnvironment.threadManager().startTimer(
                    m_executorEnvironment.executorConfig().successfulExecutionDelayMs(),
                    [this] {
                        auto tx = createMultisigTransactionInfo(*m_successfulEndBatchOpinion,
                                                                m_otherSuccessfulExecutorEndBatchOpinions);
                        sendEndBatchTransaction(tx);
                    });

        }
    } else if (m_unsuccessfulEndBatchOpinion &&
               enoughOpinions(m_otherUnsuccessfulExecutorEndBatchOpinions.size() + 1,
                              m_contractEnvironment.executors().size() + 1)) {

        if (!m_unsuccessfulEndBatchSent) {

            m_unsuccessfulEndBatchSent = true;

            m_unsuccessfulApprovalExpectationTimer = m_executorEnvironment.threadManager().startTimer(
                    m_executorEnvironment.executorConfig().unsuccessfulExecutionDelayMs(),
                    [this] {
                        auto tx = createMultisigTransactionInfo(*m_unsuccessfulEndBatchOpinion,
                                                                m_otherUnsuccessfulExecutorEndBatchOpinions);
                        sendEndBatchTransaction(tx);
                    });
        }
    }
}

blockchain::SuccessfulEndBatchExecutionTransactionInfo
BatchExecutionTask::createMultisigTransactionInfo(const SuccessfulEndBatchExecutionOpinion& transactionOpinion,
                                                  std::map<ExecutorKey, SuccessfulEndBatchExecutionOpinion> otherTransactionOpinions) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    blockchain::SuccessfulEndBatchExecutionTransactionInfo multisigTransactionInfo;

    // Fill common information
    multisigTransactionInfo.m_contractKey = transactionOpinion.m_contractKey;
    multisigTransactionInfo.m_batchIndex = transactionOpinion.m_batchIndex;
    multisigTransactionInfo.m_automaticExecutionsCheckedUpTo = transactionOpinion.m_automaticExecutionsCheckedUpTo;
    multisigTransactionInfo.m_successfulBatchInfo = transactionOpinion.m_successfulBatchInfo;

    for (const auto& call: transactionOpinion.m_callsExecutionInfo) {
        blockchain::SuccessfulCallExecutionInfo callInfo;
        callInfo.m_callId = call.m_callId;
        callInfo.m_manual = call.m_manual;
        callInfo.m_block = call.m_block;
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

blockchain::UnsuccessfulEndBatchExecutionTransactionInfo
BatchExecutionTask::createMultisigTransactionInfo(const UnsuccessfulEndBatchExecutionOpinion& transactionOpinion,
                                                  std::map<ExecutorKey, UnsuccessfulEndBatchExecutionOpinion> otherTransactionOpinions) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    blockchain::UnsuccessfulEndBatchExecutionTransactionInfo multisigTransactionInfo;

    // Fill common information
    multisigTransactionInfo.m_contractKey = transactionOpinion.m_contractKey;
    multisigTransactionInfo.m_batchIndex = transactionOpinion.m_batchIndex;
    multisigTransactionInfo.m_automaticExecutionsCheckedUpTo = transactionOpinion.m_automaticExecutionsCheckedUpTo;

    for (const auto& call: transactionOpinion.m_callsExecutionInfo) {
        blockchain::UnsuccessfulCallExecutionInfo callInfo;
        callInfo.m_callId = call.m_callId;
        callInfo.m_manual = call.m_manual;
        callInfo.m_block = call.m_block;
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

void BatchExecutionTask::sendEndBatchTransaction(const blockchain::SuccessfulEndBatchExecutionTransactionInfo& transactionInfo) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.logger().debug("Send successful end batch execution transaction. "
                                         "Contract key: {}, batch index: {}",
                                         m_contractEnvironment.contractKey(),
                                         m_batch.m_batchIndex);

    m_executorEnvironment.executorEventHandler().endBatchTransactionIsReady(transactionInfo);
}

void BatchExecutionTask::sendEndBatchTransaction(const blockchain::UnsuccessfulEndBatchExecutionTransactionInfo& transactionInfo) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.logger().debug("Send successful end batch execution transaction. "
                                         "Contract key: {}, batch index: {}",
                                         m_contractEnvironment.contractKey(),
                                         m_batch.m_batchIndex);

    m_executorEnvironment.executorEventHandler().endBatchTransactionIsReady(transactionInfo);
}

void BatchExecutionTask::executeNextCall() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_callExecutionManager, m_executorEnvironment.logger())

    ASSERT(!m_storageQuery, m_executorEnvironment.logger())

    if (m_callIterator != m_batch.m_callRequests.end()) {
        auto callRequest = *m_callIterator;
        m_callIterator++;

        auto[query, callback] = createAsyncQuery<std::unique_ptr<storage::SandboxModification>>(
                [=, this, callRequest = std::move(callRequest)](auto&& res) mutable {
                    if (!res) {
                        const auto& ec = res.error();
                        ASSERT(ec == storage::StorageError::storage_unavailable, m_executorEnvironment.logger())
                        onUnableToExecuteBatch(ec);
                        return;
                    }

                    onInitiatedSandboxModification(std::move(*res), std::move(callRequest));
                }, [] {}, m_executorEnvironment, true, true);

        m_storageQuery = std::move(query);

        m_storageModification->initiateSandboxModification({m_executorEnvironment.executorConfig().storagePathPrefix()},
                                                           callback);
    } else {
        computeProofOfExecution();

        auto[query, callback] = createAsyncQuery<storage::StorageState>([this](auto&& state) {

            if (!state) {
                const auto& ec = state.error();
                ASSERT(ec == storage::StorageError::storage_unavailable, m_executorEnvironment.logger())
                onUnableToExecuteBatch(ec);
                return;
            }

            onStorageHashEvaluated(std::move(*state));
        }, [] {}, m_executorEnvironment, true, true);

        m_storageQuery = std::move(query);

        m_storageModification->evaluateStorageHash(callback);
    }
}

void BatchExecutionTask::onUnsuccessfulExecutionTimerExpiration() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_successfulEndBatchOpinion, m_executorEnvironment.logger())
    ASSERT(!m_unsuccessfulEndBatchOpinion, m_executorEnvironment.logger())

    m_unsuccessfulEndBatchOpinion = UnsuccessfulEndBatchExecutionOpinion();
    m_unsuccessfulEndBatchOpinion->m_contractKey = m_successfulEndBatchOpinion->m_contractKey;
    m_unsuccessfulEndBatchOpinion->m_batchIndex = m_successfulEndBatchOpinion->m_batchIndex;
    m_unsuccessfulEndBatchOpinion->m_automaticExecutionsCheckedUpTo = m_successfulEndBatchOpinion->m_automaticExecutionsCheckedUpTo;
    m_unsuccessfulEndBatchOpinion->m_proof = m_contractEnvironment.proofOfExecution().buildPreviousProof();
    m_unsuccessfulEndBatchOpinion->m_executorKey = m_successfulEndBatchOpinion->m_executorKey;


    m_unsuccessfulEndBatchOpinion->m_callsExecutionInfo.reserve(
            m_successfulEndBatchOpinion->m_callsExecutionInfo.size());
    for (const auto& successfulCall: m_successfulEndBatchOpinion->m_callsExecutionInfo) {
        UnsuccessfulCallExecutionOpinion unsuccessfulCall;
        unsuccessfulCall.m_callId = successfulCall.m_callId;
        unsuccessfulCall.m_manual = successfulCall.m_manual;
        unsuccessfulCall.m_block = successfulCall.m_block;
        unsuccessfulCall.m_executorParticipation = successfulCall.m_executorParticipation;
        m_unsuccessfulEndBatchOpinion->m_callsExecutionInfo.push_back(unsuccessfulCall);
    }

    m_unsuccessfulEndBatchOpinion->sign(m_executorEnvironment.keyPair());

    checkEndBatchTransactionReadiness();

    // The opinion will be shared when the share opinion timer ticks
}

void BatchExecutionTask::computeProofOfExecution() {

}

void BatchExecutionTask::onUnableToExecuteBatch(const std::error_code& ec) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.logger().error("Unable to execute batch. "
                                         "Contract key: {}, batch index: {}. "
                                         "Reason: {}",
                                         m_contractEnvironment.contractKey(),
                                         m_batch.m_batchIndex,
                                         ec.message());

    m_contractEnvironment.batchesManager().delayBatch(Batch(m_batch));

    m_finished = true;

    m_unableToExecuteBatchTimer = Timer(m_executorEnvironment.threadManager().context(),
                                        m_executorEnvironment.executorConfig().serviceUnavailableTimeoutMs(), [this] {
                m_onTaskFinishedCallback->postReply(expected<void>());
            });
}

void BatchExecutionTask::onAppliedStorageModifications() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_storageQuery, m_executorEnvironment.logger())

    m_storageQuery.reset();

    const auto& cosigners = m_publishedEndBatchInfo->m_cosigners;
    if (std::find(cosigners.begin(), cosigners.end(), m_executorEnvironment.keyPair().publicKey()) ==
        cosigners.end()) {
        blockchain::EndBatchExecutionSingleTransactionInfo singleTx = {m_contractEnvironment.contractKey(),
                                                           m_batch.m_batchIndex,
                                                           m_contractEnvironment.proofOfExecution().buildActualProof()};
        m_executorEnvironment.executorEventHandler().endBatchSingleTransactionIsReady(singleTx);
    }

    m_finished = true;

    m_onTaskFinishedCallback->postReply(expected<void>());
}

void BatchExecutionTask::shareOpinions() {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto messenger = m_executorEnvironment.messenger().lock();

    m_executorEnvironment.logger().debug("Share opinions. "
                                         "Contract key: {}, batch index: {}. ",
                                         m_contractEnvironment.contractKey(),
                                         m_batch.m_batchIndex);

    if (messenger) {

        ASSERT(m_successfulEndBatchOpinion, m_executorEnvironment.logger())

        for (const auto&[executor, _]: m_contractEnvironment.executors()) {
            auto serializedInfo = utils::serialize(*m_successfulEndBatchOpinion);
            auto tag = magic_enum::enum_name(MessageTag::SUCCESSFUL_END_BATCH);
            messenger->sendMessage(messenger::OutputMessage{executor, {tag.begin(), tag.end()}, serializedInfo});
        }

        if (m_unsuccessfulEndBatchOpinion) {
            for (const auto&[executor, _]: m_contractEnvironment.executors()) {
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

bool BatchExecutionTask::onBlockPublished(uint64_t height) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto& batchesManager = m_contractEnvironment.batchesManager();

    if (!batchesManager.isBatchValid(m_batch)) {
        ASSERT(!m_publishedEndBatchInfo, m_executorEnvironment.logger());
        m_contractEnvironment.batchesManager().delayBatch(Batch(m_batch));

        m_finished = true;

        m_onTaskFinishedCallback->postReply(expected<void>());
    }

    return true;
}

}