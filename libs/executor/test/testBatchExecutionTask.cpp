///*
//*** Copyright 2021 ProximaX Limited. All rights reserved.
//*** Use of this source code is governed by the Apache 2.0
//*** license that can be found in the LICENSE file.
//*/
//
//#include "gtest/gtest.h"
//#include "BatchExecutionTask.h"
//#include "ContractEnvironmentMock.h"
//#include "VirtualMachineMock.h"
//#include "ExecutorEnvironmentMock.h"
//#include "utils/Random.h"
//#include "StorageMock.h"
//#include "MessengerMock.h"
//
//#include <cereal/types/vector.hpp>
//
//namespace sirius::contract::test {
//#define TEST_NAME BatchExecutionTaskTest
//
//TEST(TEST_NAME, PublishedBatchNotSynchronizedTest) {
//    // create executor environment
//    crypto::PrivateKey privateKey;
//    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
//    ThreadManager threadManager;
//    std::deque<bool> result = {true, true, true};
//    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result);
//    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;
//    ExecutorConfig executorConfig;
//    auto storageMock = std::make_shared<StorageMock>();
//    std::weak_ptr<StorageMock> pStorageMock = storageMock;
//    auto messengerMock = std::make_shared<MessengerMock>();
//    std::weak_ptr<MessengerMock> pMessengerMock = messengerMock;
//
//    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
//                                                    threadManager, pStorageMock, pMessengerMock);
//    // create batch
//    std::deque<std::shared_ptr<CallRequest>> callRequests;
//    for (auto i = 0; i < 1; i++) {
//        auto request = std::make_shared<ManualCallRequest>(
//                utils::generateRandomByteValue<CallId>(),
//                "",
//                "",
//                52000000,
//                20 * 1024,
//                CallerKey(),
//                0,
//                std::vector<uint8_t>(),
//                std::vector<ServicePayment>());
//        callRequests.push_back(request);
//    }
//
//    Batch batch = {
//            1,
//            0,
//            callRequests
//    };
//    // create contract environment mock
//    ContractKey contractKey;
//    uint64_t automaticExecutionsSCLimit = 0;
//    uint64_t automaticExecutionsSMLimit = 0;
//    std::shared_ptr<ContractEnvironment> pContractEnvironmentMock;
//
//    // create batch execution task
//    std::map<ExecutorKey, SuccessfulEndBatchExecutionOpinion> otherSuccessfulExecutorEndBatchOpinions;
//    std::map<ExecutorKey, UnsuccessfulEndBatchExecutionOpinion> otherUnsuccessfulExecutorEndBatchOpinions;
//    std::unique_ptr<BaseContractTask> pBatchExecutionTask;
//
//    // create published end batch txn info
//    PublishedEndBatchExecutionTransactionInfo publishedEndBatchTxnInfo;
//    publishedEndBatchTxnInfo.m_contractKey = contractKey;
//    publishedEndBatchTxnInfo.m_batchIndex = 1;
//    publishedEndBatchTxnInfo.m_batchSuccess = true;
//    publishedEndBatchTxnInfo.m_driveState = pStorageMock.lock()->m_storageHash;
//    publishedEndBatchTxnInfo.m_cosigners.insert(utils::generateRandomByteValue<ExecutorKey>());
//
//    std::promise<void> barrier;
//    threadManager.execute([&] {
//        pContractEnvironmentMock = std::make_unique<ContractEnvironmentMock>(
//                executorEnvironmentMock, contractKey, automaticExecutionsSCLimit, automaticExecutionsSMLimit);
//
//        pBatchExecutionTask = std::make_unique<BatchExecutionTask>(std::move(batch),
//                                                                   *pContractEnvironmentMock,
//                                                                   executorEnvironmentMock,
//                                                                   std::move(otherSuccessfulExecutorEndBatchOpinions),
//                                                                   std::move(otherUnsuccessfulExecutorEndBatchOpinions),
//                                                                   std::nullopt);
//
//        pBatchExecutionTask->run();
//        barrier.set_value();
//    });
//    sleep(3);
//    threadManager.execute([&] {
//        ASSERT_TRUE(pBatchExecutionTask->onEndBatchExecutionPublished(publishedEndBatchTxnInfo));
//    });
//    sleep(1);
//    ASSERT_EQ(std::future_status::ready, barrier.get_future().wait_for(std::chrono::seconds(5)));
//
//    threadManager.execute([&] {
//        pBatchExecutionTask.reset();
//    });
//
//    threadManager.stop();
//}
//
//TEST(TEST_NAME, EndBatchExecutionFailedTest) {
//    // create executor environment
//    crypto::PrivateKey privateKey;
//    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
//    ThreadManager threadManager;
//    std::deque<bool> result = {true, true, true};
//    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result);
//    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;
//    ExecutorConfig executorConfig;
//    executorConfig.setSuccessfulExecutionDelayMs(1000);
//    auto storageMock = std::make_shared<StorageMock>();
//    std::weak_ptr<StorageMock> pStorageMock = storageMock;
//    auto messengerMock = std::make_shared<MessengerMock>();
//    std::weak_ptr<MessengerMock> pMessengerMock = messengerMock;
//
//    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
//                                                    threadManager, pStorageMock, pMessengerMock);
//    // create batch
//    std::deque<std::shared_ptr<CallRequest>> callRequests;
//    for (auto i = 0; i < 1; i++) {
//        auto request = std::make_shared<ManualCallRequest>(
//                utils::generateRandomByteValue<CallId>(),
//                "",
//                "",
//                52000000,
//                20 * 1024,
//                CallerKey(),
//                0,
//                std::vector<uint8_t>(),
//                std::vector<ServicePayment>());
//        callRequests.push_back(request);
//    }
//
//    Batch batch = {
//            1,
//            0,
//            callRequests
//    };
//    // create contract environment mock
//    ContractKey contractKey;
//    uint64_t automaticExecutionsSCLimit = 0;
//    uint64_t automaticExecutionsSMLimit = 0;
//    std::shared_ptr<ContractEnvironment> pContractEnvironmentMock;
//
//    // create batch execution task
//    std::map<ExecutorKey, SuccessfulEndBatchExecutionOpinion> otherSuccessfulExecutorEndBatchOpinions;
//    std::map<ExecutorKey, UnsuccessfulEndBatchExecutionOpinion> otherUnsuccessfulExecutorEndBatchOpinions;
//    std::unique_ptr<BaseContractTask> pBatchExecutionTask;
//
//    // create published end batch txn info
//    FailedEndBatchExecutionTransactionInfo failedEndBatchTxnInfo;
//    failedEndBatchTxnInfo.m_contractKey = contractKey;
//    failedEndBatchTxnInfo.m_batchIndex = 1;
//    failedEndBatchTxnInfo.m_batchSuccess = true;
//
//    // create opinions
//    std::vector<SuccessfulCallExecutionOpinion> callsExecutionOpinions;
//    for (const auto& call: callRequests) {
//        callsExecutionOpinions.push_back(SuccessfulCallExecutionOpinion{
//            call->callId(),
//            call->isManual(),
//            call->blockHeight(),
//            0,
//            TransactionHash(),
//            CallExecutorParticipation{
//                call->executionPayment(),
//                call->downloadPayment()
//            }
//        });
//    }
//    std::vector<SuccessfulEndBatchExecutionOpinion> opinionList;
//    for (int i = 0; i < 2; i++) {
//        SuccessfulEndBatchExecutionOpinion opinion;
//        opinion.m_batchIndex = 1;
//        opinion.m_contractKey = contractKey;
//        opinion.m_executorKey = utils::generateRandomByteValue<ExecutorKey>();
//        opinion.m_callsExecutionInfo = callsExecutionOpinions;
//        SuccessfulBatchInfo successfulBatchInfo{
//            pStorageMock.lock()->m_storageHash,
//            0,
//            0,
//            {}
//        };
//        opinion.m_successfulBatchInfo = successfulBatchInfo;
//        opinionList.push_back(opinion);
//    }
//
//    std::promise<void> barrier;
//    threadManager.execute([&] {
//        pContractEnvironmentMock = std::make_unique<ContractEnvironmentMock>(
//                executorEnvironmentMock, contractKey, automaticExecutionsSCLimit, automaticExecutionsSMLimit);
//
//        pBatchExecutionTask = std::make_unique<BatchExecutionTask>(std::move(batch),
//                                                                   *pContractEnvironmentMock,
//                                                                   executorEnvironmentMock,
//                                                                   std::move(
//                                                                           otherSuccessfulExecutorEndBatchOpinions),
//                                                                           std::move(
//                                                                                   otherUnsuccessfulExecutorEndBatchOpinions),
//                                                                                   std::nullopt);
//
//        pBatchExecutionTask->run();
//        opinionList[0].m_successfulBatchInfo.m_PoExVerificationInfo = pContractEnvironmentMock
//                ->proofOfExecution().addToProof(0);
//        ASSERT_TRUE(pBatchExecutionTask->onEndBatchExecutionOpinionReceived(opinionList[0]));
//        opinionList[1].m_successfulBatchInfo.m_PoExVerificationInfo = pContractEnvironmentMock
//                ->proofOfExecution().addToProof(0);
//        ASSERT_TRUE(pBatchExecutionTask->onEndBatchExecutionOpinionReceived(opinionList[1]));
//
//        barrier.set_value();
//    });
//    sleep(5);
//    threadManager.execute([&] {
//        ASSERT_TRUE(pBatchExecutionTask->onEndBatchExecutionFailed(failedEndBatchTxnInfo));
//    });
//    sleep(2);
//    ASSERT_EQ(std::future_status::ready, barrier.get_future().wait_for(std::chrono::seconds(5)));
//
//    threadManager.execute([&] {
//        pBatchExecutionTask.reset();
//    });
//
//    threadManager.stop();
//}
//
//TEST(TEST_NAME, TerminateTest) {
//    // create executor environment
//    crypto::PrivateKey privateKey;
//    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
//    ThreadManager threadManager;
//    std::deque<bool> result = {true, true, true};
//    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result);
//    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;
//    ExecutorConfig executorConfig;
//    auto storageMock = std::make_shared<StorageMock>();
//    std::weak_ptr<StorageMock> pStorageMock = storageMock;
//    auto messengerMock = std::make_shared<MessengerMock>();
//    std::weak_ptr<MessengerMock> pMessengerMock = messengerMock;
//
//    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
//                                                    threadManager, pStorageMock, pMessengerMock);
//    // create batch
//    std::deque<std::shared_ptr<CallRequest>> callRequests;
//    for (auto i = 0; i < 2; i++) {
//        auto request = std::make_shared<ManualCallRequest>(
//                utils::generateRandomByteValue<CallId>(),
//                "",
//                "",
//                52000000,
//                20 * 1024,
//                CallerKey(),
//                0,
//                std::vector<uint8_t>(),
//                std::vector<ServicePayment>());
//        callRequests.push_back(request);
//    }
//
//    Batch batch = {
//            1,
//            0,
//            callRequests
//    };
//    // create contract environment mock
//    ContractKey contractKey;
//    uint64_t automaticExecutionsSCLimit = 0;
//    uint64_t automaticExecutionsSMLimit = 0;
//    std::shared_ptr<ContractEnvironment> pContractEnvironmentMock;
//
//    // create batch execution task
//    std::map<ExecutorKey, SuccessfulEndBatchExecutionOpinion> otherSuccessfulExecutorEndBatchOpinions;
//    std::map<ExecutorKey, UnsuccessfulEndBatchExecutionOpinion> otherUnsuccessfulExecutorEndBatchOpinions;
//    std::unique_ptr<BaseContractTask> pBatchExecutionTask;
//
//    // create failed end batch txn info
//    FailedEndBatchExecutionTransactionInfo failedEndBatchTxnInfo;
//    failedEndBatchTxnInfo.m_contractKey = contractKey;
//    failedEndBatchTxnInfo.m_batchIndex = 1;
//    failedEndBatchTxnInfo.m_batchSuccess = true;
//
//    // create published end batch txn info
//    PublishedEndBatchExecutionTransactionInfo publishedEndBatchTxnInfo;
//    publishedEndBatchTxnInfo.m_contractKey = contractKey;
//    publishedEndBatchTxnInfo.m_batchIndex = 1;
//    publishedEndBatchTxnInfo.m_batchSuccess = true;
//    publishedEndBatchTxnInfo.m_driveState = pStorageMock.lock()->m_storageHash;
//    publishedEndBatchTxnInfo.m_cosigners.insert(utils::generateRandomByteValue<ExecutorKey>());
//
//    // create opinions
//    std::vector<SuccessfulCallExecutionOpinion> callsExecutionOpinions;
//    for (const auto& call: callRequests) {
//        callsExecutionOpinions.push_back(SuccessfulCallExecutionOpinion{
//            call->callId(),
//            call->isManual(),
//            call->blockHeight(),
//            0,
//            TransactionHash(),
//            CallExecutorParticipation{
//                call->executionPayment(),
//                call->downloadPayment()
//            }
//        });
//    }
//    std::vector<SuccessfulEndBatchExecutionOpinion> opinionList;
//    for (int i = 0; i < 2; i++) {
//        SuccessfulEndBatchExecutionOpinion opinion;
//        opinion.m_batchIndex = 1;
//        opinion.m_contractKey = contractKey;
//        opinion.m_executorKey = utils::generateRandomByteValue<ExecutorKey>();
//        opinion.m_callsExecutionInfo = callsExecutionOpinions;
//        SuccessfulBatchInfo successfulBatchInfo{
//            pStorageMock.lock()->m_storageHash,
//            0,
//            0,
//            {}
//            // poexVerificationInfo
//        };
//        opinion.m_successfulBatchInfo = successfulBatchInfo;
//        opinionList.push_back(opinion);
//    }
//
//    std::promise<void> barrier;
//    threadManager.execute([&] {
//        pContractEnvironmentMock = std::make_unique<ContractEnvironmentMock>(
//                executorEnvironmentMock, contractKey, automaticExecutionsSCLimit, automaticExecutionsSMLimit);
//
//        pBatchExecutionTask = std::make_unique<BatchExecutionTask>(std::move(batch),
//                                                                   *pContractEnvironmentMock,
//                                                                   executorEnvironmentMock,
//                                                                   std::move(
//                                                                           otherSuccessfulExecutorEndBatchOpinions),
//                                                                           std::move(
//                                                                                   otherUnsuccessfulExecutorEndBatchOpinions),
//                                                                                   std::nullopt);
//
//        pBatchExecutionTask->run();
//        pBatchExecutionTask->terminate();
//        opinionList[0].m_successfulBatchInfo.m_PoExVerificationInfo = pContractEnvironmentMock
//                ->proofOfExecution().addToProof(0);
//        ASSERT_FALSE(pBatchExecutionTask->onEndBatchExecutionOpinionReceived(opinionList[0]));
//        opinionList[1].m_successfulBatchInfo.m_PoExVerificationInfo = pContractEnvironmentMock
//                ->proofOfExecution().addToProof(0);
//        ASSERT_FALSE(pBatchExecutionTask->onEndBatchExecutionOpinionReceived(opinionList[1]));
//        ASSERT_FALSE(pBatchExecutionTask->onEndBatchExecutionPublished(publishedEndBatchTxnInfo));
//        ASSERT_FALSE(pBatchExecutionTask->onEndBatchExecutionFailed(failedEndBatchTxnInfo));
//
//        barrier.set_value();
//    });
//
//    ASSERT_EQ(std::future_status::ready, barrier.get_future().wait_for(std::chrono::seconds(5)));
//
//    threadManager.execute([&] {
//        pBatchExecutionTask.reset();
//    });
//
//    threadManager.stop();
//}
//}