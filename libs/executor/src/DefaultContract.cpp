/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "DefaultContract.h"

#include "BatchesManager.h"
#include "BaseContractTask.h"
#include "ProofOfExecution.h"
#include "ContractConfig.h"
#include "ExecutorEnvironment.h"
#include "DefaultBatchesManager.h"

#include "InitContractTask.h"
#include "SynchronizationTask.h"
#include "BatchExecutionTask.h"
#include "RemoveContractTask.h"

namespace sirius::contract {

DefaultContract::DefaultContract(const ContractKey& contractKey,
                                 AddContractRequest&& addContractRequest,
                                 ExecutorEnvironment& contractContext)
        : m_contractKey(contractKey)
          , m_driveKey(addContractRequest.m_driveKey)
          , m_executors(std::move(addContractRequest.m_executors))
          , m_automaticExecutionsSCLimit(addContractRequest.m_automaticExecutionsSCLimit)
          , m_automaticExecutionsSMLimit(addContractRequest.m_automaticExecutionsSMLimit)
          , m_executorEnvironment(contractContext)
          , m_contractConfig()
          , m_proofOfExecution(m_executorEnvironment.keyPair(),
                               m_executorEnvironment.executorConfig().maxBatchesHistorySize())
          , m_batchesManager(std::make_unique<DefaultBatchesManager>(
                addContractRequest.m_recentBatchesInformation.empty()
                ? 0 : (--addContractRequest.m_recentBatchesInformation.end())->first,
                *this,
                m_executorEnvironment)) {
    m_contractConfig.setAutomaticExecutionsFile(addContractRequest.m_automaticExecutionsFileName);
    m_contractConfig.setAutomaticExecutionsFunction(addContractRequest.m_automaticExecutionsFunctionName);
    runInitializeContractTask(std::move(addContractRequest));
}

void DefaultContract::terminate() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_task) {
        m_task->terminate();
    }
}

void DefaultContract::addManualCall(const ManualCallRequest& request) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("New manual call is added. Contract key: {}, call id: {}", m_contractKey,
                                        request.callId());

    m_batchesManager->addManualCall(request);
}

void
DefaultContract::removeContract(const RemoveRequest& request, std::shared_ptr<AsyncQueryCallback<void>>&& callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_contractRemoveRequest = {request, std::move(callback)};

    if (m_task) {
        m_task->terminate();
    } else {
        runTask();
    }
}

void DefaultContract::setExecutors(std::map<ExecutorKey, ExecutorInfo>&& executors) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executors = std::move(executors);
}

bool DefaultContract::onBlockPublished(uint64_t blockHeight) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("New block is added. Contract key: {}, block height: {}",
                                        m_contractKey,
                                        blockHeight);

    m_batchesManager->addBlock(blockHeight);

    if (m_task) {
        m_task->onBlockPublished(blockHeight);
    }

    return true;
}

void DefaultContract::setAutomaticExecutionsEnabledSince(uint64_t blockHeight) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_batchesManager->setAutomaticExecutionsEnabledSince(blockHeight);
}

// region blockchain event handler

bool DefaultContract::onEndBatchExecutionPublished(const blockchain::PublishedEndBatchExecutionTransactionInfo& info) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("End batch execution is published. Contract  key: {}, batch index: {}",
                                        m_contractKey,
                                        info.m_batchIndex);

    m_proofOfExecution.addBatchVerificationInformation(info.m_batchIndex, info.m_PoExVerificationInfo);

    m_batchesManager->fixUnmodifiable(info.m_automaticExecutionsCheckedUpTo);
    m_batchesManager->setAutomaticExecutionsEnabledSince(info.m_automaticExecutionsEnabledSince);

    if (!m_task || !m_task->onEndBatchExecutionPublished(info)) {
        m_unknownPublishedEndBatchTransactions[info.m_batchIndex] = info;
    }

    clearOutdatedCache(m_unknownPublishedEndBatchTransactions, m_batchesManager->minBatchIndex());

    ASSERT(info.m_batchIndex > m_lastKnownStorageState.m_batchIndex, m_executorEnvironment.logger())

    m_lastKnownStorageState = {info.m_batchIndex, info.m_driveState};

    m_executorEnvironment.logger().debug("Updated last known storage state. Contract key: {}, storage state: {}",
                                         m_contractKey,
                                         info.m_driveState);

    return true;
}

bool DefaultContract::onEndBatchExecutionFailed(const blockchain::FailedEndBatchExecutionTransactionInfo& info) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (!m_task) {
        return false;
    }

    m_executorEnvironment.logger().warn("End batch execution transaction failed. Contract key: {}", m_contractKey);

    return m_task->onEndBatchExecutionFailed(info);
}

bool DefaultContract::onStorageSynchronizedPublished(uint64_t batchIndex) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_task) {
        m_task->onStorageSynchronizedPublished(batchIndex);
    }

    return true;
}

// endregion

// region message event handler

bool DefaultContract::onEndBatchExecutionOpinionReceived(const SuccessfulEndBatchExecutionOpinion& info) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_unknownSuccessfulBatchOpinions[info.m_batchIndex][info.m_executorKey] = info;

    if (m_task) {
        m_task->onEndBatchExecutionOpinionReceived(info);
    }

    clearOutdatedCache(m_unknownSuccessfulBatchOpinions, batchesManager().minBatchIndex());

    return true;
}

bool DefaultContract::onEndBatchExecutionOpinionReceived(const UnsuccessfulEndBatchExecutionOpinion& info) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_unknownUnsuccessfulBatchOpinions[info.m_batchIndex][info.m_executorKey] = info;

    if (m_task) {
        m_task->onEndBatchExecutionOpinionReceived(info);
    }

    clearOutdatedCache(m_unknownUnsuccessfulBatchOpinions, batchesManager().minBatchIndex());

    return true;
}

// endregion

// region task context

const ContractKey& DefaultContract::contractKey() const {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    return m_contractKey;
}

const std::map<ExecutorKey, ExecutorInfo>& DefaultContract::executors() const {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    return m_executors;
}

const DriveKey& DefaultContract::driveKey() const {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    return m_driveKey;
}

uint64_t DefaultContract::automaticExecutionsSCLimit() const {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    return m_automaticExecutionsSCLimit;
}

uint64_t DefaultContract::automaticExecutionsSMLimit() const {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    return m_automaticExecutionsSMLimit;
}

const ContractConfig& DefaultContract::contractConfig() const {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    return m_contractConfig;
}

void DefaultContract::addSynchronizationTask() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_synchronizationRequest = m_lastKnownStorageState;
}

void DefaultContract::notifyHasNextBatch() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.logger().debug("Has next batch. Contract key: {}", m_contractKey);

    if (!m_task) {
        runTask();
    }
}

// endregion

void DefaultContract::runTask() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_task.reset();

    if (m_contractRemoveRequest) {
        runRemoveContractTask();
    } else if (m_synchronizationRequest) {
        runSynchronizationTask();
    } else if (m_batchesManager->hasNextBatch()) {
        runBatchExecutionTask();
    }
}

void DefaultContract::runInitializeContractTask(AddContractRequest&& request) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto[_, callback] = createAsyncQuery<void>([this](auto&&) {
        runTask();
    }, [] {}, m_executorEnvironment, false, false);

    m_task = std::make_unique<InitContractTask>(std::move(request), std::move(callback), *this, m_executorEnvironment);

    m_contractRemoveRequest.reset();

    m_task->run();
}

void DefaultContract::runRemoveContractTask() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_contractRemoveRequest, m_executorEnvironment.logger())

    m_task = std::make_unique<RemoveContractTask>(std::move(m_contractRemoveRequest->m_request),
                                                  std::move(m_contractRemoveRequest->m_callback),
                                                  *this,
                                                  m_executorEnvironment);

    m_contractRemoveRequest.reset();

    m_task->run();
}

void DefaultContract::runSynchronizationTask() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_synchronizationRequest, m_executorEnvironment.logger())

    auto[_, callback] = createAsyncQuery<void>([this](auto&&) {
        runTask();
    }, [] {}, m_executorEnvironment, false, false);

    m_task = std::make_unique<SynchronizationTask>(std::move(*m_synchronizationRequest),
                                                   std::move(callback),
                                                   *this,
                                                   m_executorEnvironment);

    m_synchronizationRequest.reset();

    m_task->run();
}

BaseBatchesManager& DefaultContract::batchesManager() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    return *m_batchesManager;
}

ProofOfExecution& DefaultContract::proofOfExecution() {
    return m_proofOfExecution;
}

void DefaultContract::runBatchExecutionTask() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_batchesManager->hasNextBatch(), m_executorEnvironment.logger())

    auto batch = m_batchesManager->nextBatch();

    auto[_, callback] = createAsyncQuery<void>([this](auto&&) {
        runTask();
    }, [] {}, m_executorEnvironment, false, false);

    auto batchIndex = batch.m_batchIndex;

    m_task = std::make_unique<BatchExecutionTask>(std::move(batch),
                                                  std::move(callback),
                                                  *this,
                                                  m_executorEnvironment);
    m_task->run();

    {
        auto successfulExecutorsEndBatchOpinionsIt = m_unknownSuccessfulBatchOpinions.find(batchIndex);
        if (successfulExecutorsEndBatchOpinionsIt != m_unknownSuccessfulBatchOpinions.end()) {
            for (const auto&[key, opinion]: successfulExecutorsEndBatchOpinionsIt->second) {
                m_task->onEndBatchExecutionOpinionReceived(opinion);
            }
        }
    }

    {
        std::map<ExecutorKey, UnsuccessfulEndBatchExecutionOpinion> unsuccessfulEndBatchOpinions;
        auto unsuccessfulExecutorsEndBatchOpinionsIt = m_unknownUnsuccessfulBatchOpinions.find(batchIndex);
        if (unsuccessfulExecutorsEndBatchOpinionsIt != m_unknownUnsuccessfulBatchOpinions.end()) {
            for (const auto&[key, opinion]: unsuccessfulExecutorsEndBatchOpinionsIt->second) {
                m_task->onEndBatchExecutionOpinionReceived(opinion);
            }
        }
    }

    {
        auto publishedTransactionInfoIt = m_unknownPublishedEndBatchTransactions.find(batchIndex);
        if (publishedTransactionInfoIt != m_unknownPublishedEndBatchTransactions.end()) {
            m_task->onEndBatchExecutionPublished(publishedTransactionInfoIt->second);
        }
    }
}

}