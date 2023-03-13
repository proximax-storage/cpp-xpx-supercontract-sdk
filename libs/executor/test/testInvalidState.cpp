/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "gtest/gtest.h"
#include "BatchExecutionTask.h"
#include "ContractEnvironmentMock.h"
#include "VirtualMachineMock.h"
#include "ExecutorEnvironmentMock.h"
#include "utils/Random.h"
#include "StorageMock.h"
#include "MessengerMock.h"
#include "TestUtils.h"
#include <utils/IntegerMath.h>

#include <cereal/types/vector.hpp>

namespace sirius::contract::test {

namespace {

class ExecutorEventHandlerTestMock : public ExecutorEventHandlerMock {

public:

    blockchain::SuccessfulEndBatchExecutionTransactionInfo m_expectedInfo;
    std::promise<blockchain::SuccessfulEndBatchExecutionTransactionInfo> m_successfulEndBatchIsReadyPromise;

    void endBatchTransactionIsReady(const blockchain::SuccessfulEndBatchExecutionTransactionInfo& info) override {
        ASSERT_EQ(info.m_contractKey, m_expectedInfo.m_contractKey);
        ASSERT_EQ(info.m_batchIndex, m_expectedInfo.m_batchIndex);
        ASSERT_EQ(info.m_automaticExecutionsCheckedUpTo, m_expectedInfo.m_automaticExecutionsCheckedUpTo);
        ASSERT_EQ(info.m_successfulBatchInfo, m_expectedInfo.m_successfulBatchInfo);
        auto expectedKeysArgSort = argSort(m_expectedInfo.m_executorKeys);
        ASSERT_EQ(info.m_executorKeys.size(), m_expectedInfo.m_executorKeys.size());
        for (uint i = 0; i < info.m_executorKeys.size(); i++) {
            ASSERT_EQ(info.m_executorKeys[i], m_expectedInfo.m_executorKeys[expectedKeysArgSort[i]]);
        }
        ASSERT_EQ(info.m_callsExecutionInfo.size(), m_expectedInfo.m_callsExecutionInfo.size());
        for (uint i = 0; i < info.m_callsExecutionInfo.size(); i++) {
            const auto& actualCallInfo = info.m_callsExecutionInfo[i];
            const auto& expectedCallInfo = m_expectedInfo.m_callsExecutionInfo[i];
            ASSERT_EQ(actualCallInfo.m_callId, expectedCallInfo.m_callId);
            ASSERT_EQ(actualCallInfo.m_manual, expectedCallInfo.m_manual);
            ASSERT_EQ(actualCallInfo.m_block, expectedCallInfo.m_block);
            ASSERT_EQ(actualCallInfo.m_callExecutionStatus, expectedCallInfo.m_callExecutionStatus);
            ASSERT_EQ(actualCallInfo.m_releasedTransaction, expectedCallInfo.m_releasedTransaction);
            const auto& actualExecutorParticipation = actualCallInfo.m_executorsParticipation;
            const auto& expectedExecutorParticipation = expectedCallInfo.m_executorsParticipation;
            ASSERT_EQ(actualExecutorParticipation.size(), expectedExecutorParticipation.size());
            for (uint j = 0; j < actualExecutorParticipation.size(); j++) {
                if (actualExecutorParticipation[j].m_scConsumed !=
                    expectedExecutorParticipation[expectedKeysArgSort[j]].m_scConsumed) {
                }
                ASSERT_EQ(actualExecutorParticipation[j].m_scConsumed,
                          expectedExecutorParticipation[expectedKeysArgSort[j]].m_scConsumed);
                ASSERT_EQ(actualExecutorParticipation[j].m_smConsumed,
                          expectedExecutorParticipation[expectedKeysArgSort[j]].m_smConsumed);
            }
        }
        m_successfulEndBatchIsReadyPromise.set_value(info);
    }

    void endBatchTransactionIsReady(const blockchain::UnsuccessfulEndBatchExecutionTransactionInfo& info) override {
        FAIL();
    }

    void endBatchSingleTransactionIsReady(const blockchain::EndBatchExecutionSingleTransactionInfo& info) override {
        FAIL();
    }

    void synchronizationSingleTransactionIsReady(const blockchain::SynchronizationSingleTransactionInfo& info) override {
        FAIL();
    }

    void releasedTransactionsAreReady(const blockchain::SerializedAggregatedTransaction& payloads) override {
        FAIL();
    }

};

}

TEST(BatchExecutionTask, InvalidState) {

    const uint otherExecutorsNumber = 3;
    const uint otherOpinionsNumber = 2;
    const uint callsNumber = 2;

    crypto::PrivateKey privateKey = sirius::crypto::PrivateKey::Generate([] {
        return utils::generateRandomByteValue<uint8_t>();
    });
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ThreadManager threadManager;

    ExecutorConfig executorConfig;
    executorConfig.setSuccessfulExecutionDelayMs(1000);

    std::deque<std::shared_ptr<CallRequest>> callRequests;
    for (uint i = 0; i < callsNumber; i++) {
        auto request = std::make_shared<ManualCallRequest>(
                utils::generateRandomByteValue<CallId>(),
                "",
                "",
                52000000,
                20 * 1024,
                CallerKey(),
                0,
                std::vector<uint8_t>(),
                std::vector<ServicePayment>());
        callRequests.push_back(request);
    }

    std::deque<vm::CallExecutionResult> results;
    for (const auto& callRequest: callRequests) {
        vm::CallExecutionResult result;
        result.m_success = true;
        result.m_return = 0;
        result.m_execution_gas_consumed = utils::generateRandomByteValue<uint64_t>()
                                          % (callRequest->executionPayment() *
                                             executorConfig.downloadPaymentToGasMultiplier());
        result.m_download_gas_consumed = utils::generateRandomByteValue<uint64_t>()
                                         % (callRequest->downloadPayment() *
                                            executorConfig.downloadPaymentToGasMultiplier());
        result.m_proofOfExecutionSecretData = utils::generateRandomByteValue<uint64_t>();
        results.push_back(result);
    }

    Batch batch = {
            0,
            0,
            callRequests
    };

    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, results, 3000U);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;
    auto storageMock = std::make_shared<StorageMock>();
    auto messengerMock = std::make_shared<MessengerMock>();
    std::weak_ptr<MessengerMock> pMessengerMock = messengerMock;

    auto executorEventHandler = std::make_shared<ExecutorEventHandlerTestMock>();

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager, storageMock, pMessengerMock,
                                                    executorEventHandler);
    ContractKey contractKey = utils::generateRandomByteValue<ContractKey>();
    std::shared_ptr<ContractEnvironmentMock> pContractEnvironmentMock =  std::make_unique<ContractEnvironmentMock>(
            executorEnvironmentMock, contractKey, 0, 0);

    std::vector<sirius::crypto::KeyPair> executorKeys;
    for (uint i = 0; i < otherExecutorsNumber; i++) {
        executorKeys.push_back(
                sirius::crypto::KeyPair::FromPrivate(sirius::crypto::PrivateKey::Generate([] {
                    return utils::generateRandomByteValue<uint8_t>();
                })));
    }

    for (uint i = 0; i < executorKeys.size(); i++) {
        pContractEnvironmentMock->m_executors.try_emplace(
                executorKeys[i].publicKey().array(), ExecutorInfo{});
    }

    std::vector<SuccessfulEndBatchExecutionOpinion> opinionList;

    blockchain::SuccessfulEndBatchExecutionTransactionInfo expectedInfo;
    expectedInfo.m_contractKey = contractKey;
    expectedInfo.m_batchIndex = batch.m_batchIndex;
    expectedInfo.m_automaticExecutionsCheckedUpTo = batch.m_automaticExecutionsCheckedUpTo;
    expectedInfo.m_executorKeys.emplace_back(executorEnvironmentMock.keyPair().publicKey());
    expectedInfo.m_signatures.emplace_back();
    expectedInfo.m_proofs.emplace_back();

    storage::StorageState initialState = storageMock->m_info->m_state;
    storage::StorageState expectedState = storageMock->m_info->m_state;
    for (uint i = 0; i < callsNumber; i++) {
        expectedState = nextState(expectedState);
    }
    expectedInfo.m_successfulBatchInfo.m_storageHash = expectedState.m_storageHash;
    expectedInfo.m_successfulBatchInfo.m_usedStorageSize = expectedState.m_usedDriveSize;
    expectedInfo.m_successfulBatchInfo.m_metaFilesSize = expectedState.m_metaFilesSize;
    expectedInfo.m_successfulBatchInfo.m_PoExVerificationInfo =
            ProofOfExecution::verificationInfo(results.back().m_proofOfExecutionSecretData).second;

    for (uint i = 0; i < callRequests.size(); i++) {
        const auto& call = callRequests[i];
        blockchain::SuccessfulCallExecutionInfo callExecutionInfo;
        callExecutionInfo.m_block = call->blockHeight();
        callExecutionInfo.m_callId = call->callId();
        callExecutionInfo.m_manual = call->isManual();
        callExecutionInfo.m_callExecutionStatus = 0;
        callExecutionInfo.m_executorsParticipation.push_back({
                                                                     utils::DivideCeil(
                                                                             results[i].m_execution_gas_consumed,
                                                                             executorConfig.executionPaymentToGasMultiplier()),
                                                                     utils::DivideCeil(
                                                                             results[i].m_download_gas_consumed,
                                                                             executorConfig.downloadPaymentToGasMultiplier())});
        expectedInfo.m_callsExecutionInfo.push_back(callExecutionInfo);
    }

    for (uint i = 0; i < otherOpinionsNumber; i++) {
        SuccessfulEndBatchExecutionOpinion opinion;
        opinion.m_batchIndex = 0;
        opinion.m_contractKey = contractKey;
        opinion.m_executorKey = executorKeys[i].publicKey().array();
        std::vector<SuccessfulCallExecutionOpinion> callsExecutionOpinions;
        for (const auto& call: callRequests) {
            callsExecutionOpinions.push_back(SuccessfulCallExecutionOpinion{
                    call->callId(),
                    call->isManual(),
                    call->blockHeight(),
                    0,
                    TransactionHash(),
                    blockchain::CallExecutorParticipation{
                            utils::generateRandomByteValue<uint64_t>() % call->executionPayment(),
                            utils::generateRandomByteValue<uint64_t>() % call->downloadPayment()
                    }
            });
        }
        opinion.m_callsExecutionInfo = callsExecutionOpinions;
        ProofOfExecution proofOfExecutionBuilder(executorKeys[i]);
        auto verificationInfo = proofOfExecutionBuilder.addToProof(results.back().m_proofOfExecutionSecretData);
        blockchain::SuccessfulBatchInfo successfulBatchInfo{
                expectedState.m_storageHash,
                expectedState.m_usedDriveSize,
                expectedState.m_metaFilesSize,
                verificationInfo
        };
        opinion.m_successfulBatchInfo = successfulBatchInfo;
        opinion.m_proof = proofOfExecutionBuilder.buildActualProof();
        opinionList.push_back(opinion);
        expectedInfo.m_executorKeys.push_back(opinion.m_executorKey);
        expectedInfo.m_signatures.emplace_back();
        expectedInfo.m_proofs.emplace_back();
        for (uint j = 0; j < callRequests.size(); j++) {
            expectedInfo.m_callsExecutionInfo[j].m_executorsParticipation.push_back(
                    {callsExecutionOpinions[j].m_executorParticipation});
        }
    }

    executorEventHandler->m_expectedInfo = expectedInfo;

    std::unique_ptr<BaseContractTask> pBatchExecutionTask;
    threadManager.execute([&] {

        auto[_, callback] = createAsyncQuery<void>([](auto&&) {}, [] {}, executorEnvironmentMock, false, false);

        pBatchExecutionTask = std::make_unique<BatchExecutionTask>(std::move(batch),
                                                                   std::move(callback),
                                                                   *pContractEnvironmentMock,
                                                                   executorEnvironmentMock);
        pBatchExecutionTask->run();
        for (const auto& opinion: opinionList) {
            ASSERT_TRUE(pBatchExecutionTask->onEndBatchExecutionOpinionReceived(opinion));
        }
    });

    auto barrier = executorEventHandler->m_successfulEndBatchIsReadyPromise.get_future();
    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    auto transactionInfo = barrier.get();

    blockchain::PublishedEndBatchExecutionTransactionInfo publishedInfo;
    publishedInfo.m_contractKey = transactionInfo.m_contractKey;
    publishedInfo.m_batchIndex = transactionInfo.m_batchIndex;
    publishedInfo.m_batchSuccess = true;
    publishedInfo.m_PoExVerificationInfo = transactionInfo.m_successfulBatchInfo.m_PoExVerificationInfo;
    // The published state differs from the one obtained by the executor, so we force it to synchronize
    publishedInfo.m_driveState = randomState().m_storageHash;
    publishedInfo.m_automaticExecutionsCheckedUpTo = transactionInfo.m_automaticExecutionsCheckedUpTo;
    publishedInfo.m_automaticExecutionsEnabledSince = 0;
    publishedInfo.m_cosigners = {transactionInfo.m_executorKeys.begin(), transactionInfo.m_executorKeys.end()};

    threadManager.execute([&] {
        pBatchExecutionTask->onEndBatchExecutionPublished(publishedInfo);
    });

    auto synchronizeBarrier = pContractEnvironmentMock->m_synchronizationPromise.get_future();
    ASSERT_EQ(std::future_status::ready, synchronizeBarrier.wait_for(std::chrono::seconds(5)));

    {
        std::promise<void> storageStatePromise;
        threadManager.execute([&] {
            ASSERT_EQ(storageMock->m_info->m_state.m_storageHash, initialState.m_storageHash);
            ASSERT_EQ(storageMock->m_info->m_historicBatches.size(), 0);
            storageStatePromise.set_value();
        });
        ASSERT_EQ(std::future_status::ready, storageStatePromise.get_future().wait_for(std::chrono::seconds(5)));
    }

    threadManager.execute([&] {
        pBatchExecutionTask.reset();
    });

    threadManager.stop();
}

}