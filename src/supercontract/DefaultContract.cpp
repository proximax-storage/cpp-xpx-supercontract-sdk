/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "DefaultContract.h"

#include "BatchesManager.h"
#include "ContractEnvironment.h"
#include "BaseContractTask.h"
#include "ProofOfExecution.h"
#include "ContractConfig.h"
#include "ExecutorEnvironment.h"
#include "TaskRequests.h"
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
        , m_batchesManager(std::make_unique<DefaultBatchesManager>(addContractRequest.m_batchesExecuted, *this,
                                                                   m_executorEnvironment)) {
    runInitializeContractTask(std::move(addContractRequest));
}

void DefaultContract::terminate() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_task) {
        m_task->terminate();
    }
}

void DefaultContract::addContractCall(const CallRequest& request) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_batchesManager->addCall(request);
    if (!m_task) {
        runTask();
    }
}

void DefaultContract::removeContract(const RemoveRequest& request) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_task) {
        m_task->terminate();
    } else {
        runTask();
    }
}

void DefaultContract::setExecutors(std::set<ExecutorKey>&& executors) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executors = std::move(executors);
}

void DefaultContract::addBlockInfo(const Block& block) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_batchesManager->addBlockInfo(block);
}

void DefaultContract::setAutomaticExecutionsEnabledSince(const std::optional<uint64_t>& blockHeight) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_batchesManager->setAutomaticExecutionsEnabledSince(blockHeight);
}

// region blockchain event handler

bool DefaultContract::onEndBatchExecutionPublished(const PublishedEndBatchExecutionTransactionInfo& info) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    while (!m_unknownSuccessfulBatchOpinions.empty()
           && m_unknownSuccessfulBatchOpinions.begin()->first <= info.m_batchIndex) {
        m_unknownSuccessfulBatchOpinions.erase(m_unknownSuccessfulBatchOpinions.begin());
    }

    while (!m_unknownUnsuccessfulBatchOpinions.empty()
           && m_unknownUnsuccessfulBatchOpinions.begin()->first <= info.m_batchIndex) {
        m_unknownUnsuccessfulBatchOpinions.erase(m_unknownUnsuccessfulBatchOpinions.begin());
    }

    if (!m_task || !m_task->onEndBatchExecutionPublished(info)) {
        m_unknownPublishedEndBatchTransactions[info.m_batchIndex] = info;
    }

    ASSERT(info.m_batchIndex > m_lastKnownStorageState.m_batchIndex, m_executorEnvironment.logger())

    m_lastKnownStorageState = {info.m_batchIndex, info.m_driveState};

    return true;
}

bool DefaultContract::onEndBatchExecutionFailed(const FailedEndBatchExecutionTransactionInfo& info) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (!m_task) {
        return false;
    }

    return m_task->onEndBatchExecutionFailed(info);
}

bool DefaultContract::onStorageSynchronized(uint64_t batchIndex) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_batchesManager->onStorageSynchronized(batchIndex);

    if (!m_task) {
        return false;
    }

    m_task->onStorageSynchronized(batchIndex);

    // TODO should we always return true?
    return true;
}

// endregion

// region message event handler

bool DefaultContract::onEndBatchExecutionOpinionReceived(const EndBatchExecutionOpinion& info) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (!m_task || !m_task->onEndBatchExecutionOpinionReceived(info)) {
        if (info.isSuccessful()) {
            m_unknownSuccessfulBatchOpinions[info.m_batchIndex][info.m_executorKey] = info;
        } else {
            m_unknownUnsuccessfulBatchOpinions[info.m_batchIndex][info.m_executorKey] = info;
        }
    }

    return true;
}

// endregion

// region task context

const ContractKey& DefaultContract::contractKey() const {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    return m_contractKey;
}

const std::set<ExecutorKey>& DefaultContract::executors() const {

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

void DefaultContract::finishTask() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    runTask();
}

void DefaultContract::addSynchronizationTask() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_synchronizationRequest = {m_lastKnownStorageState.m_storageHash};
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

    m_task = std::make_unique<InitContractTask>(std::move(request), *this, m_executorEnvironment);

    m_contractRemoveRequest.reset();

    m_task->run();
}

void DefaultContract::runRemoveContractTask() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_contractRemoveRequest, m_executorEnvironment.logger())

    m_task = std::make_unique<RemoveContractTask>(std::move(*m_contractRemoveRequest), *this, m_executorEnvironment);

    m_contractRemoveRequest.reset();

    m_task->run();
}

void DefaultContract::runSynchronizationTask() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_synchronizationRequest, m_executorEnvironment.logger())

    m_task = std::make_unique<SynchronizationTask>(std::move(*m_synchronizationRequest), *this, m_executorEnvironment);

    m_synchronizationRequest.reset();

    m_task->run();
}

void DefaultContract::runBatchExecutionTask() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_batchesManager->hasNextBatch(), m_executorEnvironment.logger())

    auto batch = m_batchesManager->nextBatch();

    std::map<ExecutorKey, EndBatchExecutionOpinion> successfulEndBatchOpinions;
    auto successfulExecutorsEndBatchOpinionsIt = m_unknownSuccessfulBatchOpinions.find(batch.m_batchIndex);
    if (successfulExecutorsEndBatchOpinionsIt != m_unknownSuccessfulBatchOpinions.end()) {
        successfulEndBatchOpinions = std::move(successfulExecutorsEndBatchOpinionsIt->second);
        m_unknownSuccessfulBatchOpinions.erase(successfulExecutorsEndBatchOpinionsIt);
    }

    std::map<ExecutorKey, EndBatchExecutionOpinion> unsuccessfulEndBatchOpinions;
    auto unsuccessfulExecutorsEndBatchOpinionsIt = m_unknownUnsuccessfulBatchOpinions.find(batch.m_batchIndex);
    if (unsuccessfulExecutorsEndBatchOpinionsIt != m_unknownUnsuccessfulBatchOpinions.end()) {
        unsuccessfulEndBatchOpinions = std::move(unsuccessfulExecutorsEndBatchOpinionsIt->second);
        m_unknownUnsuccessfulBatchOpinions.erase(unsuccessfulExecutorsEndBatchOpinionsIt);
    }

    std::optional<PublishedEndBatchExecutionTransactionInfo> publishedInfo;
    auto publishedTransactionInfoIt = m_unknownPublishedEndBatchTransactions.find(batch.m_batchIndex);
    if (publishedTransactionInfoIt != m_unknownPublishedEndBatchTransactions.end()) {
        publishedInfo = std::move(publishedTransactionInfoIt->second);
        m_unknownPublishedEndBatchTransactions.erase(publishedTransactionInfoIt);
    }

    m_task = std::make_unique<BatchExecutionTask>(std::move(batch), *this, m_executorEnvironment,
                                                  std::move(successfulEndBatchOpinions),
                                                  std::move(unsuccessfulEndBatchOpinions),
                                                  std::move(publishedInfo));
    m_task->run();
}

}