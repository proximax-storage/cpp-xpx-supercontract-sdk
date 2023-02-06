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
#include <utils/IntegerMath.h>
#include <magic_enum.hpp>

#include <cereal/types/vector.hpp>

namespace sirius::contract::test {

namespace {

class ExecutorEventHandlerTestMock : public ExecutorEventHandlerMock {

public:

    void endBatchTransactionIsReady(const SuccessfulEndBatchExecutionTransactionInfo& info) override {
    }

    void endBatchTransactionIsReady(const UnsuccessfulEndBatchExecutionTransactionInfo& info) override {
        FAIL();
    }

    void endBatchSingleTransactionIsReady(const EndBatchExecutionSingleTransactionInfo& info) override {
        FAIL();
    }

    void synchronizationSingleTransactionIsReady(const SynchronizationSingleTransactionInfo& info) override {
        FAIL();
    }

    void releasedTransactionsAreReady(const std::vector<std::vector<uint8_t>>& payloads) override {
        FAIL();
    }

};

}

TEST(BatchExecutionTask, OpinionMessages) {

    const uint otherExecutorsNumber = 3;
    const uint otherOpinionsNumber = 2;
    const uint callsNumber = 2;
    const uint messagesNumber = 3;
    const uint unsuccessfulOpinionDelay = 1000;
    const uint messagesPeriodicity = 3000; // must be more than 1000 for test correctness;

    crypto::PrivateKey privateKey = sirius::crypto::PrivateKey::Generate([] {
        return utils::generateRandomByteValue<uint8_t>();
    });
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ThreadManager threadManager;

    ExecutorConfig executorConfig;
    executorConfig.setShareOpinionTimeoutMs(messagesPeriodicity);

    std::deque<std::shared_ptr<CallRequest>> callRequests;
    for (auto i = 0; i < callsNumber; i++) {
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

    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, results, 0U);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;
    auto storageMock = std::make_shared<StorageMock>();
    auto messengerMock = std::make_shared<MessengerMock>();
    std::weak_ptr<MessengerMock> pMessengerMock = messengerMock;

    auto executorEventHandler = std::make_shared<ExecutorEventHandlerTestMock>();

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager, storageMock, pMessengerMock,
                                                    executorEventHandler);
    ContractKey contractKey = utils::generateRandomByteValue<ContractKey>();
    ContractConfig contractConfig;
    contractConfig.setUnsuccessfulApprovalDelayMs(unsuccessfulOpinionDelay);
    std::shared_ptr<ContractEnvironmentMock> pContractEnvironmentMock = std::make_unique<ContractEnvironmentMock>(
            executorEnvironmentMock, contractKey, 0, 0, contractConfig);

    std::vector<sirius::crypto::KeyPair> executorKeys;
    for (int i = 0; i < otherExecutorsNumber; i++) {
        executorKeys.push_back(
                sirius::crypto::KeyPair::FromPrivate(sirius::crypto::PrivateKey::Generate([] {
                    return utils::generateRandomByteValue<uint8_t>();
                })));
    }

    for (int i = 0; i < executorKeys.size(); i++) {
        pContractEnvironmentMock->m_executors.try_emplace(
                executorKeys[i].publicKey().array(), ExecutorInfo{});
    }

    std::vector<SuccessfulEndBatchExecutionOpinion> opinionList;

    SuccessfulEndBatchExecutionOpinion expectedOpinion;
    expectedOpinion.m_contractKey = contractKey;
    expectedOpinion.m_batchIndex = batch.m_batchIndex;
    expectedOpinion.m_automaticExecutionsCheckedUpTo = batch.m_automaticExecutionsCheckedUpTo;
    expectedOpinion.m_executorKey = executorEnvironmentMock.keyPair().publicKey().array();

    ProofOfExecution proofOfExecutionBuilder(executorEnvironmentMock.keyPair());
    auto expectedVerificationInfo = proofOfExecutionBuilder.addToProof(results.back().m_proofOfExecutionSecretData);
    expectedOpinion.m_proof = proofOfExecutionBuilder.buildActualProof();

    storage::StorageState expectedState = storageMock->m_state;
    for (uint i = 0; i < callsNumber; i++) {
        expectedState = nextState(expectedState);
    }
    expectedOpinion.m_successfulBatchInfo.m_storageHash = expectedState.m_storageHash;
    expectedOpinion.m_successfulBatchInfo.m_usedStorageSize = expectedState.m_usedDriveSize;
    expectedOpinion.m_successfulBatchInfo.m_metaFilesSize = expectedState.m_metaFilesSize;
    expectedOpinion.m_successfulBatchInfo.m_PoExVerificationInfo =
            ProofOfExecution::verificationInfo(results.back().m_proofOfExecutionSecretData).second;

    for (uint i = 0; i < callRequests.size(); i++) {
        const auto& call = callRequests[i];
        SuccessfulCallExecutionOpinion callExecutionOpinion;
        callExecutionOpinion.m_block = call->blockHeight();
        callExecutionOpinion.m_callId = call->callId();
        callExecutionOpinion.m_manual = call->isManual();
        callExecutionOpinion.m_callExecutionStatus = 0;
        callExecutionOpinion.m_executorParticipation = {
                utils::DivideCeil(
                        results[i].m_execution_gas_consumed,
                        executorConfig.executionPaymentToGasMultiplier()),
                utils::DivideCeil(
                        results[i].m_download_gas_consumed,
                        executorConfig.downloadPaymentToGasMultiplier())};
        expectedOpinion.m_callsExecutionInfo.push_back(callExecutionOpinion);
    }

    for (int i = 0; i < 0; i++) {
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
                    CallExecutorParticipation{
                            utils::generateRandomByteValue<uint64_t>() % call->executionPayment(),
                            utils::generateRandomByteValue<uint64_t>() % call->downloadPayment()
                    }
            });
        }
        opinion.m_callsExecutionInfo = callsExecutionOpinions;
        ProofOfExecution opinionProofOfExecutionBuilder(executorKeys[i]);
        auto verificationInfo = opinionProofOfExecutionBuilder.addToProof(results.back().m_proofOfExecutionSecretData);
        SuccessfulBatchInfo successfulBatchInfo{
                expectedState.m_storageHash,
                expectedState.m_usedDriveSize,
                expectedState.m_metaFilesSize,
                verificationInfo
        };
        opinion.m_successfulBatchInfo = successfulBatchInfo;
        opinion.m_proof = opinionProofOfExecutionBuilder.buildActualProof();
        opinionList.push_back(opinion);
    }

    std::unique_ptr<BaseContractTask> pBatchExecutionTask;
    threadManager.execute([&] {
        pBatchExecutionTask = std::make_unique<BatchExecutionTask>(std::move(batch),
                                                                   *pContractEnvironmentMock,
                                                                   executorEnvironmentMock,
                                                                   std::map<ExecutorKey, SuccessfulEndBatchExecutionOpinion>(),
                                                                   std::map<ExecutorKey, UnsuccessfulEndBatchExecutionOpinion>(),
                                                                   std::nullopt);
        pBatchExecutionTask->run();
        for (const auto& opinion: opinionList) {
            ASSERT_TRUE(pBatchExecutionTask->onEndBatchExecutionOpinionReceived(opinion));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds((messagesNumber - 1) * messagesPeriodicity + 1000));

    threadManager.execute([&] {
        pBatchExecutionTask.reset();

        ASSERT_EQ(executorKeys.size(), messengerMock->m_sentMessages.size());
        for (const auto& keyPair: executorKeys) {
            auto executorIt = messengerMock->m_sentMessages.find(keyPair.publicKey().array());
            ASSERT_NE(executorIt, messengerMock->m_sentMessages.end());
            const auto& messagesByTags = executorIt->second;
            ASSERT_EQ(messagesByTags.size(), 2);
            {
                auto tag = std::string(magic_enum::enum_name(MessageTag::SUCCESSFUL_END_BATCH));
                auto messagesIt = messagesByTags.find(tag);
                ASSERT_NE(messagesIt, messagesByTags.end());
                const auto& messages = messagesIt->second;
                ASSERT_EQ(messages.size(), messagesNumber);
                for (const auto& message: messages) {
                    auto opinion = utils::deserialize<SuccessfulEndBatchExecutionOpinion>(message);
                    ASSERT_TRUE(opinion.verify());
                    ASSERT_EQ(opinion.m_executorKey, expectedOpinion.m_executorKey);
                    ASSERT_EQ(opinion.m_contractKey, expectedOpinion.m_contractKey);
                    ASSERT_EQ(opinion.m_batchIndex, expectedOpinion.m_batchIndex);
                    ASSERT_EQ(opinion.m_automaticExecutionsCheckedUpTo,
                              expectedOpinion.m_automaticExecutionsCheckedUpTo);
                    ASSERT_EQ(opinion.m_successfulBatchInfo, expectedOpinion.m_successfulBatchInfo);

                    ASSERT_EQ(opinion.m_callsExecutionInfo.size(), expectedOpinion.m_callsExecutionInfo.size());
                    for (int i = 0; i < opinion.m_callsExecutionInfo.size(); i++) {
                        const auto& actualCallInfo = opinion.m_callsExecutionInfo[i];
                        const auto& expectedCallInfo = expectedOpinion.m_callsExecutionInfo[i];
                        ASSERT_EQ(actualCallInfo.m_callId, expectedCallInfo.m_callId);
                        ASSERT_EQ(actualCallInfo.m_manual, expectedCallInfo.m_manual);
                        ASSERT_EQ(actualCallInfo.m_block, expectedCallInfo.m_block);
                        ASSERT_EQ(actualCallInfo.m_callExecutionStatus, expectedCallInfo.m_callExecutionStatus);
                        ASSERT_EQ(actualCallInfo.m_releasedTransaction, expectedCallInfo.m_releasedTransaction);
                        ASSERT_EQ(actualCallInfo.m_executorParticipation.m_scConsumed,
                                  expectedCallInfo.m_executorParticipation.m_scConsumed);
                        ASSERT_EQ(actualCallInfo.m_executorParticipation.m_smConsumed,
                                  expectedCallInfo.m_executorParticipation.m_smConsumed);
                    }
                }
            }
            {
                auto tag = std::string(magic_enum::enum_name(MessageTag::UNSUCCESSFUL_END_BATCH));
                auto messagesIt = messagesByTags.find(tag);
                ASSERT_NE(messagesIt, messagesByTags.end());
                const auto& messages = messagesIt->second;
                ASSERT_EQ(messages.size(), messagesNumber - 1);
                for (const auto& message: messages) {
                    auto opinion = utils::deserialize<UnsuccessfulEndBatchExecutionOpinion>(message);
                    ASSERT_TRUE(opinion.verify());
                    ASSERT_EQ(opinion.m_executorKey, expectedOpinion.m_executorKey);
                    auto expectedProof = proofOfExecutionBuilder.buildPreviousProof();
                    ASSERT_EQ(opinion.m_proof.m_initialBatch, expectedProof.m_initialBatch);
                    ASSERT_EQ(opinion.m_proof.m_batchProof.m_T, expectedProof.m_batchProof.m_T);
                    ASSERT_EQ(opinion.m_proof.m_batchProof.m_r, expectedProof.m_batchProof.m_r);
                    ASSERT_EQ(opinion.m_proof.m_tProof.m_F, expectedProof.m_tProof.m_F);
                    ASSERT_EQ(opinion.m_proof.m_tProof.m_k, expectedProof.m_tProof.m_k);
                    ASSERT_EQ(opinion.m_contractKey, expectedOpinion.m_contractKey);
                    ASSERT_EQ(opinion.m_batchIndex, expectedOpinion.m_batchIndex);
                    ASSERT_EQ(opinion.m_automaticExecutionsCheckedUpTo,
                              expectedOpinion.m_automaticExecutionsCheckedUpTo);

                    ASSERT_EQ(opinion.m_callsExecutionInfo.size(), expectedOpinion.m_callsExecutionInfo.size());
                    for (int i = 0; i < opinion.m_callsExecutionInfo.size(); i++) {
                        const auto& actualCallInfo = opinion.m_callsExecutionInfo[i];
                        const auto& expectedCallInfo = expectedOpinion.m_callsExecutionInfo[i];
                        ASSERT_EQ(actualCallInfo.m_callId, expectedCallInfo.m_callId);
                        ASSERT_EQ(actualCallInfo.m_manual, expectedCallInfo.m_manual);
                        ASSERT_EQ(actualCallInfo.m_block, expectedCallInfo.m_block);
                        ASSERT_EQ(actualCallInfo.m_executorParticipation.m_scConsumed,
                                  expectedCallInfo.m_executorParticipation.m_scConsumed);
                        ASSERT_EQ(actualCallInfo.m_executorParticipation.m_smConsumed,
                                  expectedCallInfo.m_executorParticipation.m_smConsumed);
                    }
                }
            }
        }
    });

    threadManager.stop();
}

}