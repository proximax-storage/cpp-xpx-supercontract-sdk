/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "DefaultBatchesManager.h"
#include <virtualMachine/ExecutionErrorConidition.h>
#include <crypto/Hashes.h>
#include <utils/Serializer.h>
#include "AutorunBlockchainQueryHandler.h"

namespace sirius::contract {

DefaultBatchesManager::DefaultBatchesManager(uint64_t nextBatchIndex, ContractEnvironment& contractEnvironment,
                                             ExecutorEnvironment& executorEnvironment)
        : m_contractEnvironment(contractEnvironment)
          , m_executorEnvironment(executorEnvironment)
          , m_nextBatchIndex(nextBatchIndex) {}

void DefaultBatchesManager::run() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (!m_run) {
        m_run = true;
        for(const auto& callInfo: m_autorunCallInfos) {
            callInfo.second.m_callExecutionManager->run();
        }
    }
}

void DefaultBatchesManager::addManualCall(const ManualCallRequest& request) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto height = request.blockHeight();

    if (m_batches.empty() || (--m_batches.end())->first != height) {
        m_batches.try_emplace(height);
    }

    auto batchIt = --m_batches.end();

    batchIt->second.m_requests.push_back(std::make_shared<ManualCallRequest>(request));
}

void DefaultBatchesManager::setAutomaticExecutionsEnabledSince(const std::optional<uint64_t>& blockHeight) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_automaticExecutionsEnabledSince = blockHeight;

    if (m_delayedBatch && !isBatchValid(*m_delayedBatch)) {
        delayedBatchExtractAutomaticCall();
    }

    if (!m_automaticExecutionsEnabledSince) {
        disableAutomaticExecutionsTill(UINT64_MAX);
    } else {
        disableAutomaticExecutionsTill(*m_automaticExecutionsEnabledSince);
    }
}

bool DefaultBatchesManager::hasNextBatch() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    clearOutdatedBatches();

    if (m_delayedBatch) {
        return true;
    }

    return !m_batches.empty() &&
           m_batches.begin()->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::FINISHED;
}

Batch DefaultBatchesManager::nextBatch() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    clearOutdatedBatches();

    if (m_delayedBatch) {
        auto batch = std::move(*m_delayedBatch);
        m_delayedBatch.reset();
        return batch;
    }

    auto batchIt = m_batches.extract(m_batches.begin());
    auto batch = std::move(batchIt.mapped());
    uint64_t blockHeight = batchIt.key();

    ASSERT(!batch.m_requests.empty(), m_executorEnvironment.logger())

    return Batch{m_nextBatchIndex++, blockHeight + 1, std::move(batch.m_requests)};
}

void DefaultBatchesManager::addBlock(uint64_t blockHeight) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (!m_automaticExecutionsEnabledSince || blockHeight < *m_automaticExecutionsEnabledSince) {
        // Automatic Executions Are Disabled For This Block

        if (m_batches.empty()) {
            return;
        }

        auto batchIt = --m_batches.end();

        if (batchIt->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::MANUAL) {
            batchIt->second.m_batchFormationStatus = DraftBatch::BatchFormationStatus::FINISHED;
        }

        if (hasNextBatch()) {
        	// Note that we do not check here that exactly this batch is the next batch
        	// That's why the number of notifications might not be the same as the number of ready batches
        	// due to gaps
        	m_contractEnvironment.notifyHasNextBatch();
        }

    } else {
        if (m_batches.empty() || (--m_batches.end())->first < blockHeight) {
            m_batches.try_emplace(blockHeight);
        }

        auto batchIt = --m_batches.end();
        batchIt->second.m_batchFormationStatus = DraftBatch::BatchFormationStatus::AUTOMATIC;

        CallId callId;

        std::ostringstream seedBuilder;
        seedBuilder << utils::serialize(m_contractEnvironment.contractKey());
        seedBuilder << utils::serialize(blockHeight);
        std::string seedStr = seedBuilder.str();

        std::seed_seq seed(seedStr.begin(), seedStr.end());
        std::mt19937 rng(seed);
        std::generate_n(callId.begin(), sizeof(CallId), rng);

        auto callManager = runAutorunCall(callId, blockHeight);

        m_autorunCallInfos[blockHeight] = AutorunCallInfo{callId, std::move(callManager), Timer()};
    }
}

std::unique_ptr<CallExecutionManager> DefaultBatchesManager::runAutorunCall(const CallId& callId, uint64_t height) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    vm::CallRequest request(callId,
                            m_executorEnvironment.executorConfig().getConfigByHeight(height).autorunFile,
                            m_executorEnvironment.executorConfig().getConfigByHeight(height).autorunFunction,
                            {},
                            m_executorEnvironment.executorConfig().getConfigByHeight(height).autorunSCLimit,
                            0,
                            vm::CallRequest::CallLevel::AUTORUN,
							0,
							m_contractEnvironment.driveKey(),
							m_executorEnvironment.executorConfig().getConfigByHeight(height).maxAutorunExecutableSize);

    auto[query, callback] = createAsyncQuery<vm::CallExecutionResult>([blockHeight = height, this](auto&& result) {
        if (result) {
            onSuperContractCallExecuted(blockHeight, std::move(*result));
        } else {
            onSuperContractCallFailed(blockHeight, std::move(result.error()));
        }
    }, [] {}, m_executorEnvironment, true, true);

    auto callManager = std::make_unique<CallExecutionManager>(
            m_executorEnvironment,
            std::make_shared<vm::VirtualMachineInternetQueryHandler>(),
            std::make_shared<AutorunBlockchainQueryHandler>(m_executorEnvironment, m_contractEnvironment, height),
            std::make_shared<vm::VirtualMachineStorageQueryHandler>(),
            std::move(query),
            request,
            std::move(callback));

    if (m_run) {
        callManager->run();
    }

    return callManager;
}

void DefaultBatchesManager::onSuperContractCallExecuted(uint64_t blockHeight,
                                                        vm::CallExecutionResult&& executionResult) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    auto callIt = m_autorunCallInfos.find(blockHeight);

    ASSERT(callIt != m_autorunCallInfos.end(), m_executorEnvironment.logger())

    ASSERT(callIt->second.m_callExecutionManager, m_executorEnvironment.logger())

    auto batchIt = m_batches.find(callIt->first);

    ASSERT(batchIt != m_batches.end(), m_executorEnvironment.logger())
    ASSERT(batchIt->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::AUTOMATIC,
           m_executorEnvironment.logger())

    if (executionResult.m_success && executionResult.m_return == 0) {

        auto height = callIt->first;

        Hash256 callIdH;
        sirius::crypto::Sha3_256_Builder hasher;
        hasher.update(m_contractEnvironment.contractKey());
        hasher.update({reinterpret_cast<const uint8_t*>(&height), sizeof(height)});
        hasher.final(callIdH);

        auto pRequest = std::make_shared<CallRequest>(
                CallId{callIdH.array()},
                m_contractEnvironment.contractConfig().automaticExecutionsFile(),
                m_contractEnvironment.contractConfig().automaticExecutionsFunction(),
                m_contractEnvironment.automaticExecutionsSCLimit(),
                m_contractEnvironment.automaticExecutionsSMLimit(),
                CallerKey(),
                height);

        batchIt->second.m_requests.push_back(std::move(pRequest));
    }

    batchIt->second.m_batchFormationStatus = DraftBatch::BatchFormationStatus::FINISHED;

    if (batchIt->second.m_requests.empty()) {
        m_batches.erase(batchIt);
    }

    m_autorunCallInfos.erase(callIt);

    if (hasNextBatch()) {
        // Note that we do not check here that exactly this batch is the next batch
        // That's why the number of notifications might not be the same as the number of ready batches
        // due to gaps
        m_contractEnvironment.notifyHasNextBatch();
    }
}

void DefaultBatchesManager::onSuperContractCallFailed(uint64_t blockHeight, std::error_code&& ec) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(ec == vm::ExecutionError::virtual_machine_unavailable, m_executorEnvironment.logger())

    m_executorEnvironment.logger().error("Failed to execute autorun call. Reason: {}", ec.message());

    auto callIt = m_autorunCallInfos.find(blockHeight);

    ASSERT(callIt != m_autorunCallInfos.end(), m_executorEnvironment.logger())

    ASSERT(callIt->second.m_callExecutionManager, m_executorEnvironment.logger())

    callIt->second.m_callExecutionManager.reset();
    callIt->second.m_repeatTimer.cancel();

    callIt->second.m_repeatTimer = Timer(m_executorEnvironment.threadManager().context(),
                                         m_executorEnvironment.executorConfig().serviceUnavailableTimeoutMs(),
                                         [blockHeight = callIt->first, this] {
                                             auto callIt = m_autorunCallInfos.find(blockHeight);
                                             ASSERT(callIt != m_autorunCallInfos.end(), m_executorEnvironment.logger())
                                             auto& info = callIt->second;
                                             info.m_callExecutionManager = runAutorunCall(info.m_callId, blockHeight);
                                         });
}

void DefaultBatchesManager::skipBatches(uint64_t nextBatchIndex) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.logger().debug("Skipped batches. Contract key: {}, next batch: {}",
                                         m_contractEnvironment.contractKey(),
                                         nextBatchIndex);

    m_skippedNextBatchIndex = nextBatchIndex;
}

void DefaultBatchesManager::clearOutdatedBatches() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_delayedBatch && m_delayedBatch->m_batchIndex < m_skippedNextBatchIndex) {
        // Note that we do NOT increment next batch index
        m_delayedBatch.reset();
    }

    while (!m_batches.empty() &&
           m_batches.begin()->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::FINISHED &&
           m_nextBatchIndex < m_skippedNextBatchIndex) {
        m_batches.erase(m_batches.begin());
        m_nextBatchIndex++;
    }
}

void DefaultBatchesManager::delayBatch(Batch&& batch) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_delayedBatch, m_executorEnvironment.logger())

    ASSERT(batch.m_batchIndex + 1 == m_nextBatchIndex, m_executorEnvironment.logger())

    m_executorEnvironment.logger().debug("Batch is delayed. Contract key: {}, batch index: {}",
                                         m_contractEnvironment.contractKey(),
                                         batch.m_batchIndex);

    m_delayedBatch = std::move(batch);

    if (!isBatchValid(*m_delayedBatch)) {
        delayedBatchExtractAutomaticCall();
    }
}

void DefaultBatchesManager::disableAutomaticExecutionsTill(uint64_t nextBlockHeight) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    m_executorEnvironment.logger().debug("Automatic executions are disabled. Contract key: {}, next block height: {}",
                                         m_contractEnvironment.contractKey(),
                                         nextBlockHeight);

    {
        auto itStart = m_batches.lower_bound(m_nextModifiableBlock);

        for (auto it = itStart, nextIt = it;
             it != m_batches.end() && it->first < nextBlockHeight; it = nextIt) {
            nextIt++;

            auto& batch = it->second;

            if (batch.m_batchFormationStatus == DraftBatch::BatchFormationStatus::AUTOMATIC) {
                batch.m_batchFormationStatus = DraftBatch::BatchFormationStatus::FINISHED;
            } else if (!batch.m_requests.back()->isManual()) {
                ASSERT(batch.m_batchFormationStatus == DraftBatch::BatchFormationStatus::FINISHED,
                       m_executorEnvironment.logger())
                batch.m_requests.pop_back();
            }

            if (batch.m_requests.empty()) {
                m_batches.erase(it);
            }
        }
    }
    {
        auto itStart = m_autorunCallInfos.lower_bound(m_nextModifiableBlock);

        for (auto it = itStart; it != m_autorunCallInfos.end() && it->first < nextBlockHeight;) {
            it = m_autorunCallInfos.erase(it);
        }
    }
}

void DefaultBatchesManager::fixUnmodifiable(uint64_t nextBlockHeight) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(nextBlockHeight > m_nextModifiableBlock, m_executorEnvironment.logger())

    m_executorEnvironment.logger().debug("Automatic executions are fixed unmodifiable. Contract key: {}, next block height: {}",
                                         m_contractEnvironment.contractKey(),
                                         nextBlockHeight);

    m_nextModifiableBlock = nextBlockHeight;
}

bool DefaultBatchesManager::isBatchValid(const Batch& batch) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!batch.m_callRequests.empty(), m_executorEnvironment.logger())

    const auto& lastCall = batch.m_callRequests.back();

    if (lastCall->isManual()) {
        return true;
    }

    auto callBlockHeight = lastCall->blockHeight();

    if (callBlockHeight < m_nextModifiableBlock) {
        return true;
    }

    if (m_automaticExecutionsEnabledSince && *m_automaticExecutionsEnabledSince <= callBlockHeight) {
        return true;
    }

    return false;
}

void DefaultBatchesManager::delayedBatchExtractAutomaticCall() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(m_delayedBatch, m_executorEnvironment.logger())

    ASSERT(m_delayedBatch->m_batchIndex + 1 == m_nextBatchIndex, m_executorEnvironment.logger())

    ASSERT(!m_delayedBatch->m_callRequests.back()->isManual(), m_executorEnvironment.logger())

    m_delayedBatch->m_callRequests.pop_back();

    if (m_delayedBatch->m_callRequests.empty()) {
        m_nextBatchIndex--;
        m_delayedBatch.reset();
    }
}

uint64_t DefaultBatchesManager::minBatchIndex() {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    if (m_nextBatchIndex == 0) {
        return 0;
    }

    return std::max(m_nextBatchIndex - 1, m_skippedNextBatchIndex);
}

}