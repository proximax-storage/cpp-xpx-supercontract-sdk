/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "DefaultBatchesManager.h"
#include "ContractEnvironmentMock.h"
#include "ExecutorEnvironmentMock.h"
#include "VirtualMachineMock.h"

#include "utils/Random.h"
#include "gtest/gtest.h"

namespace sirius::contract::test {
#define TEST_NAME DefaultBatchesManagerTest

using namespace blockchain;

namespace {
    constexpr uint MAX_VM_DELAY = 3000;
}
    
TEST(TEST_NAME, BatchTest) {
    // Test procedure:
    // addCall
    // addCall
    // addBLockInfo - true
    // addBlockInfo - true
    // addBlockInfo - false
    // addCall
    // addCall
    // addBlockInfo - false
    // create contract environment
    srand(time(nullptr));
    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> results = {true, true, false, false};
    std::deque<vm::CallExecutionResult> executionResults;
    for (const auto& result: results) {
        vm::CallExecutionResult callExecutionResult;
        callExecutionResult.m_success = result;
        executionResults.push_back(callExecutionResult);
    }
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, executionResults, MAX_VM_DELAY, 0);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    ContractEnvironmentMock contractEnvironmentMock(executorEnvironmentMock, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create default batches manager
    uint64_t index = 1;
    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request

    std::vector<ManualCallRequest> requests;

    const std::vector<int> callsPerBlock = {2, 0, 0, 2};

    for (uint64_t i = 0; i < 4; i++) {
        for (uint64_t j = 0; j < callsPerBlock[i]; j++) {
            std::vector<uint8_t> params;
            ManualCallRequest request(
                    utils::generateRandomByteValue<CallId>(),
                    "",
                    "",
                    52000000,
                    20 * 1024,
                    CallerKey(),
                    i + 1,
                    params,
                    {}
            );
            requests.push_back(request);
        }
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlock(1);
        batchesManager->addBlock(2);
        batchesManager->addBlock(3);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlock(4);
    });
    sleep(4);
    threadManager.execute([&] {
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 1);
        ASSERT_EQ(batch1.m_callRequests.size(), 3);
        ASSERT_TRUE(batch1.m_callRequests[0]->isManual());
        ASSERT_TRUE(batch1.m_callRequests[1]->isManual());
        ASSERT_TRUE(!batch1.m_callRequests[2]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 2);
        ASSERT_EQ(batch2.m_callRequests.size(), 1);
        ASSERT_TRUE(!batch2.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch3 = batchesManager->nextBatch();
        ASSERT_EQ(batch3.m_batchIndex, 3);
        ASSERT_EQ(batch3.m_callRequests.size(), 2);
        ASSERT_TRUE(batch3.m_callRequests[0]->isManual());
        ASSERT_TRUE(batch3.m_callRequests[1]->isManual());
        ASSERT_FALSE(batchesManager->hasNextBatch());
    });
    
    threadManager.execute([&] {
        batchesManager.reset();
    });

    threadManager.stop();
}

TEST(TEST_NAME, AllFalseTest) {
    // Test procedure:
    // addCall x8
    // addBLockInfo - false  (batch1)
    // addCall x4
    // addBlockInfo - false  (batch2)
    // addCall x2
    // addBlockInfo - false  (batch3)
    // addBlockInfo - false
    // create contract environment
    srand(time(nullptr));
    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> results = {false, false, false, false};
    std::deque<vm::CallExecutionResult> executionResults;
    for (const auto& result: results) {
        executionResults.push_back(vm::CallExecutionResult{result});
    }
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, executionResults, MAX_VM_DELAY, 0);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    ContractEnvironmentMock contractEnvironmentMock(executorEnvironmentMock, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create default batches manager
    uint64_t index = 1;
    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request

    std::vector<ManualCallRequest> requests;

    const std::vector<int> callsPerBlock = {8, 4, 2, 0};

    for (uint64_t i = 0; i < 4; i++) {
        for (uint64_t j = 0; j < callsPerBlock[i]; j++) {
            std::vector<uint8_t> params;
            ManualCallRequest request(
                    utils::generateRandomByteValue<CallId>(),
                    "",
                    "",
                    52000000,
                    20 * 1024,
                    CallerKey(),
                    i + 1,
                    params,
                    {}
            );
            requests.push_back(request);
        }
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });
    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addManualCall(requests[4]);
        batchesManager->addManualCall(requests[5]);
        batchesManager->addManualCall(requests[6]);
        batchesManager->addManualCall(requests[7]);
        batchesManager->addBlock(1);
        batchesManager->addManualCall(requests[8]);
        batchesManager->addManualCall(requests[9]);
        batchesManager->addManualCall(requests[10]);
        batchesManager->addManualCall(requests[11]);
        batchesManager->addBlock(2);
        batchesManager->addManualCall(requests[12]);
        batchesManager->addManualCall(requests[13]);
        batchesManager->addBlock(3);
        batchesManager->addBlock(4);
    });
    sleep(4);
    threadManager.execute([&] {
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 1);
        ASSERT_EQ(batch1.m_callRequests.size(), 8);
        ASSERT_TRUE(batch1.m_callRequests[0]->isManual());
        ASSERT_TRUE(batch1.m_callRequests[1]->isManual());
        ASSERT_TRUE(batch1.m_callRequests[2]->isManual());
        ASSERT_TRUE(batch1.m_callRequests[3]->isManual());
        ASSERT_TRUE(batch1.m_callRequests[4]->isManual());
        ASSERT_TRUE(batch1.m_callRequests[5]->isManual());
        ASSERT_TRUE(batch1.m_callRequests[6]->isManual());
        ASSERT_TRUE(batch1.m_callRequests[7]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 2);
        ASSERT_EQ(batch2.m_callRequests.size(), 4);
        ASSERT_TRUE(batch2.m_callRequests[0]->isManual());
        ASSERT_TRUE(batch2.m_callRequests[1]->isManual());
        ASSERT_TRUE(batch2.m_callRequests[2]->isManual());
        ASSERT_TRUE(batch2.m_callRequests[3]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch3 = batchesManager->nextBatch();
        ASSERT_EQ(batch3.m_batchIndex, 3);
        ASSERT_EQ(batch3.m_callRequests.size(), 2);
        ASSERT_TRUE(batch3.m_callRequests[0]->isManual());
        ASSERT_TRUE(batch3.m_callRequests[1]->isManual());
        ASSERT_FALSE(batchesManager->hasNextBatch());
    });

    threadManager.execute([&] {
        batchesManager.reset();
    });

    threadManager.stop();
}

TEST(TEST_NAME, StorageSynchronisedTest) {
    // Test procedure:
    // storageSynchronised = 2
    // nextBatchIndex = 1
    // addCall
    // addBLockInfo - true  (batch1)
    // addCall
    // addBlockInfo - false (batch2)
    // addCall
    // addBlockInfo - true  (batch3)
    // addCall
    // addBlockInfo - false (batch4)
    // create contract environment
    srand(time(nullptr));
    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> results = {true, false, true, false};
    std::deque<vm::CallExecutionResult> executionResults;
    for (const auto& result: results) {
        executionResults.push_back(vm::CallExecutionResult{result});
    }
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, executionResults, MAX_VM_DELAY, 0);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    ContractEnvironmentMock contractEnvironmentMock(executorEnvironmentMock, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create default batches manager
    uint64_t index = 1;
    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request

    std::vector<ManualCallRequest> requests;

    const std::vector<int> callsPerBlock = {1, 1, 1, 1};

    for (uint64_t i = 0; i < 4; i++) {
        for (uint64_t j = 0; j < callsPerBlock[i]; j++) {
            std::vector<uint8_t> params;
            ManualCallRequest request(
                    utils::generateRandomByteValue<CallId>(),
                    "",
                    "",
                    52000000,
                    20 * 1024,
                    CallerKey(),
                    i + 1,
                    params,
                    {}
            );
            requests.push_back(request);
        }
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });
    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->cancelBatchesTill(2);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlock(1);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlock(2);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlock(3);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlock(4);
    });
    sleep(4);
    threadManager.execute([&] {
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 3);
        ASSERT_EQ(batch1.m_callRequests.size(), 2);
        ASSERT_TRUE(batch1.m_callRequests[0]->isManual());
        ASSERT_TRUE(!batch1.m_callRequests[1]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 4);
        ASSERT_EQ(batch2.m_callRequests.size(), 1);
        ASSERT_TRUE(batch2.m_callRequests[0]->isManual());
        ASSERT_FALSE(batchesManager->hasNextBatch());
    });

    threadManager.execute([&] {
        batchesManager.reset();
    });

    threadManager.stop();
}

TEST(TEST_NAME, StorageSynchronisedBatchesDeclareAtMiddleTest) {
    // Test procedure:
    // nextBatchIndex = 1
    // addCall
    // addBLockInfo - true  (batch1)
    // addCall
    // addBlockInfo - false (batch2)
    // addCall
    // storageSynchronised = 2
    // addBlockInfo - true  (batch3)
    // addCall
    // addBlockInfo - false (batch4)
    // create contract environment
    srand(time(nullptr));
    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> results = {true, false, true, false};
    std::deque<vm::CallExecutionResult> executionResults;
    for (const auto& result: results) {
        executionResults.push_back(vm::CallExecutionResult{result});
    }
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, executionResults, MAX_VM_DELAY, 0);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    ContractEnvironmentMock contractEnvironmentMock(executorEnvironmentMock, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request

    std::vector<ManualCallRequest> requests;

    const std::vector<int> callsPerBlock = {1, 1, 1, 1};

    for (uint64_t i = 0; i < 4; i++) {
        for (uint64_t j = 0; j < callsPerBlock[i]; j++) {
            std::vector<uint8_t> params;
            ManualCallRequest request(
                    utils::generateRandomByteValue<CallId>(),
                    "",
                    "",
                    52000000,
                    20 * 1024,
                    CallerKey(),
                    i + 1,
                    params,
                    {}
            );
            requests.push_back(request);
        }
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlock(1);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlock(2);
        batchesManager->cancelBatchesTill(2);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlock(3);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlock(4);
    });
    sleep(4);
    threadManager.execute([&] {
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 3);
        ASSERT_EQ(batch1.m_callRequests.size(), 2);
        ASSERT_TRUE(batch1.m_callRequests[0]->isManual());
        ASSERT_TRUE(!batch1.m_callRequests[1]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 4);
        ASSERT_EQ(batch2.m_callRequests.size(), 1);
        ASSERT_TRUE(batch2.m_callRequests[0]->isManual());
        ASSERT_FALSE(batchesManager->hasNextBatch());
    });

    threadManager.execute([&] {
        batchesManager.reset();
    });

    threadManager.stop();
}

TEST(TEST_NAME, StorageSynchronisedBatchesDeclareAtEndTest) {
    // Test procedure:
    // nextBatchIndex = 1
    // addCall
    // addBLockInfo - true  (batch1)
    // addCall
    // addBlockInfo - false (batch2)
    // addCall
    // addBlockInfo - true  (batch3)
    // addCall
    // addBlockInfo - false (batch4)
    // storageSynchronised = 2
    // create contract environment
    srand(time(nullptr));
    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> results = {true, false, true, false};
    std::deque<vm::CallExecutionResult> executionResults;
    for (const auto& result: results) {
        executionResults.push_back(vm::CallExecutionResult{result});
    }
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, executionResults, MAX_VM_DELAY, 0);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    ContractEnvironmentMock contractEnvironmentMock(executorEnvironmentMock, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request

    std::vector<ManualCallRequest> requests;

    const std::vector<int> callsPerBlock = {1, 1, 1, 1};

    for (uint64_t i = 0; i < 4; i++) {
        for (uint64_t j = 0; j < callsPerBlock[i]; j++) {
            std::vector<uint8_t> params;
            ManualCallRequest request(
                    utils::generateRandomByteValue<CallId>(),
                    "",
                    "",
                    52000000,
                    20 * 1024,
                    CallerKey(),
                    i + 1,
                    params,
                    {}
            );
            requests.push_back(request);
        }
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlock(1);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlock(2);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlock(3);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlock(4);
    });
    sleep(4);
    threadManager.execute([&] {
        batchesManager->cancelBatchesTill(2);
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 3);
        ASSERT_EQ(batch1.m_callRequests.size(), 2);
        ASSERT_TRUE(batch1.m_callRequests[0]->isManual());
        ASSERT_TRUE(!batch1.m_callRequests[1]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 4);
        ASSERT_EQ(batch2.m_callRequests.size(), 1);
        ASSERT_TRUE(batch2.m_callRequests[0]->isManual());
        ASSERT_FALSE(batchesManager->hasNextBatch());
    });

    threadManager.execute([&] {
        batchesManager.reset();
    });

    threadManager.stop();
}

TEST(TEST_NAME, DisableAutomaticExecutionsEnabledSinceTest) {
    // Test procedure:
    // enabledSince = 0
    // addCall
    // addBLockInfo - true
    // addCall
    // addBLockInfo - false
    // enabledSince = null opt (disabled)
    // addCall
    // addBLockInfo - true
    // addCall
    // addBlockInfo - false
    // create contract environment
    srand(time(nullptr));
    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> results = {true, false, true, false};
    std::deque<vm::CallExecutionResult> executionResults;
    for (const auto& result: results) {
        executionResults.push_back(vm::CallExecutionResult{result});
    }
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, executionResults, MAX_VM_DELAY, 0);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    ContractEnvironmentMock contractEnvironmentMock(executorEnvironmentMock,
                                                    contractKey,
                                                    automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request

    std::vector<ManualCallRequest> requests;

    const std::vector<int> callsPerBlock = {1, 1, 1, 1};

    for (uint64_t i = 0; i < 4; i++) {
        for (uint64_t j = 0; j < callsPerBlock[i]; j++) {
            std::vector<uint8_t> params;
            ManualCallRequest request(
                    utils::generateRandomByteValue<CallId>(),
                    "",
                    "",
                    52000000,
                    20 * 1024,
                    CallerKey(),
                    i + 1,
                    params,
                    {}
            );
            requests.push_back(request);
        }
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlock(1);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlock(2);
        batchesManager->setAutomaticExecutionsEnabledSince(std::nullopt);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlock(3);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlock(4);
    });
    sleep(4);
    threadManager.execute([&] {
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 1);
        ASSERT_EQ(batch1.m_callRequests.size(), 1);
        ASSERT_TRUE(batch1.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 2);
        ASSERT_EQ(batch2.m_callRequests.size(), 1);
        ASSERT_TRUE(batch2.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch3 = batchesManager->nextBatch();
        ASSERT_EQ(batch3.m_batchIndex, 3);
        ASSERT_EQ(batch3.m_callRequests.size(), 1);
        ASSERT_TRUE(batch3.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch4 = batchesManager->nextBatch();
        ASSERT_EQ(batch4.m_batchIndex, 4);
        ASSERT_EQ(batch4.m_callRequests.size(), 1);
        ASSERT_TRUE(batch4.m_callRequests[0]->isManual());
        ASSERT_FALSE(batchesManager->hasNextBatch());
    });

    threadManager.execute([&] {
        batchesManager.reset();
    });

    threadManager.stop();
}

TEST(TEST_NAME, MultipleEnabledSinceTest) {
    // Test procedure:
    // enabledSince = 0 (enabled)
    // addBLockInfo1 - true
    // addBLockInfo2 - false
    // addCall
    // addBLockInfo3 - true
    // addCall
    // addBlockInfo4 - false
    // enabledSince = 9 (disabled at addBlock 5-8, enabled at addBlock 9-12
    // addBlockInfo5 - true
    // addBlockInfo6 - false
    // addCall
    // addBlockInfo7 - true
    // addCall
    // addBlockInfo8 - false
    // addBlockInfo9  - true
    // addBlockInfo10 - false
    // addCall
    // addBlockInfo11 - true
    // addCall
    // addBlockInfo12 - false
    // create contract environment
    ThreadManager threadManager;
    srand(time(nullptr));
    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    std::deque<bool> results = {true, false, true, false, true, false, true, false, true, false, true, false};
    std::deque<vm::CallExecutionResult> executionResults;
    for (const auto& result: results) {
        executionResults.push_back(vm::CallExecutionResult{result});
    }
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, executionResults, MAX_VM_DELAY, 0);

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), virtualMachineMock, executorConfig,
                                                    threadManager);

    ContractEnvironmentMock contractEnvironmentMock(executorEnvironmentMock,
                                                    contractKey,
                                                    automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request

    std::vector<ManualCallRequest> requests;

    const std::vector<int> callsPerBlock = {0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1};

    for (uint64_t i = 0; i < 12; i++) {
        for (uint64_t j = 0; j < callsPerBlock[i]; j++) {
            std::vector<uint8_t> params;
            ManualCallRequest request(
                    utils::generateRandomByteValue<CallId>(),
                    "",
                    "",
                    52000000,
                    20 * 1024,
                    CallerKey(),
                    i + 1,
                    params,
                    {}
                    );
            requests.push_back(request);
        }
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addBlock(1);
        batchesManager->addBlock(2);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlock(3);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlock(4);

        batchesManager->addBlock(5);
        batchesManager->addBlock(6);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlock(7);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlock(8);

        batchesManager->setUnmodifiableUpTo(5);
        batchesManager->setAutomaticExecutionsEnabledSince(9);

        batchesManager->addBlock(9);
        batchesManager->addBlock(10);
        batchesManager->addManualCall(requests[4]);
        batchesManager->addBlock(11);
        batchesManager->addManualCall(requests[5]);
        batchesManager->addBlock(12);
    });

    sleep(4);
    
    threadManager.execute([&] {
        // enabled
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 1);
        ASSERT_EQ(batch1.m_automaticExecutionsCheckedUpTo, 2);
        ASSERT_EQ(batch1.m_callRequests.size(), 1);
        ASSERT_TRUE(!batch1.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 2);
        ASSERT_EQ(batch2.m_automaticExecutionsCheckedUpTo, 4);
        ASSERT_EQ(batch2.m_callRequests.size(), 2);
        ASSERT_TRUE(batch2.m_callRequests[0]->isManual());
        ASSERT_TRUE(!batch2.m_callRequests[1]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch3 = batchesManager->nextBatch();
        ASSERT_EQ(batch3.m_batchIndex, 3);
        ASSERT_EQ(batch3.m_automaticExecutionsCheckedUpTo, 5);
        ASSERT_EQ(batch3.m_callRequests.size(), 1);
        ASSERT_TRUE(batch3.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        // disabled
        auto batch4 = batchesManager->nextBatch();
        ASSERT_EQ(batch4.m_batchIndex, 4);
        ASSERT_EQ(batch4.m_automaticExecutionsCheckedUpTo, 8);
        ASSERT_EQ(batch4.m_callRequests.size(), 1);
        ASSERT_TRUE(batch4.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch5 = batchesManager->nextBatch();
        ASSERT_EQ(batch5.m_batchIndex, 5);
        ASSERT_EQ(batch5.m_automaticExecutionsCheckedUpTo, 9);
        ASSERT_EQ(batch5.m_callRequests.size(), 1);
        ASSERT_TRUE(batch5.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        // enabled
        auto batch6 = batchesManager->nextBatch();
        ASSERT_EQ(batch6.m_batchIndex, 6);
        ASSERT_EQ(batch6.m_automaticExecutionsCheckedUpTo, 10);
        ASSERT_EQ(batch6.m_callRequests.size(), 1);
        ASSERT_TRUE(!batch6.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch7 = batchesManager->nextBatch();
        ASSERT_EQ(batch7.m_batchIndex, 7);
        ASSERT_EQ(batch7.m_automaticExecutionsCheckedUpTo, 12);
        ASSERT_EQ(batch7.m_callRequests.size(), 2);
        ASSERT_TRUE(batch7.m_callRequests[0]->isManual());
        ASSERT_TRUE(!batch7.m_callRequests[1]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch8 = batchesManager->nextBatch();
        ASSERT_EQ(batch8.m_batchIndex, 8);
        ASSERT_EQ(batch8.m_automaticExecutionsCheckedUpTo, 13);
        ASSERT_EQ(batch8.m_callRequests.size(), 1);
        ASSERT_TRUE(batch8.m_callRequests[0]->isManual());
        ASSERT_FALSE(batchesManager->hasNextBatch());
    });
    
    threadManager.execute([&] {
        batchesManager.reset();
    });

    threadManager.stop();
}

TEST(TEST_NAME, DisableThenEnableEnabledSinceTest) {
    // Test procedure:
    // enabledSince = 0         (enabled)
    // addBLockInfo1 - true
    // addBLockInfo2 - false
    // addCall
    // addBLockInfo3 - true
    // addCall
    // addBlockInfo4 - false
    // enabledSince = null ptr  (disabled)
    // addBlockInfo5 - true
    // addBlockInfo6 - false
    // addCall
    // addBlockInfo7 - true
    // addCall
    // addBlockInfo8 - false
    // enabledSince = 9         (enabled)
    // addBlockInfo9  - true
    // addBlockInfo10 - false
    // addCall
    // addBlockInfo11 - true
    // addCall
    // addBlockInfo12 - false
    // create contract environment
    srand(time(nullptr));
    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> results = {true, false, true, false, true, false, true, false, true, false, true, false};
    std::deque<vm::CallExecutionResult> executionResults;
    for (const auto& result: results) {
        executionResults.push_back(vm::CallExecutionResult{result});
    }
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, executionResults, MAX_VM_DELAY, 0);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    ContractEnvironmentMock contractEnvironmentMock(executorEnvironmentMock, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request

    std::vector<ManualCallRequest> requests;

    const std::vector<int> callsPerBlock = {0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1};

    for (uint64_t i = 0; i < 12; i++) {
        for (auto j = 0; j < callsPerBlock[i]; j++) {
            std::vector<uint8_t> params;
            ManualCallRequest request(
                    utils::generateRandomByteValue<CallId>(),
                    "",
                    "",
                    52000000,
                    20 * 1024,
                    CallerKey(),
                    i + 1,
                    params,
                    {}
            );
            requests.push_back(request);
        }
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addBlock(1);
        batchesManager->addBlock(2);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlock(3);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlock(4);
        batchesManager->setAutomaticExecutionsEnabledSince(std::nullopt);
        batchesManager->addBlock(5);
        batchesManager->addBlock(6);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlock(7);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlock(8);
        batchesManager->setAutomaticExecutionsEnabledSince(9);
        batchesManager->addBlock(9);
        batchesManager->addBlock(10);
        batchesManager->addManualCall(requests[4]);
        batchesManager->addBlock(11);
        batchesManager->addManualCall(requests[5]);
        batchesManager->addBlock(12);
    });
    sleep(4);
    threadManager.execute([&] {
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 1);
        ASSERT_EQ(batch1.m_callRequests.size(), 1);
        ASSERT_TRUE(batch1.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 2);
        ASSERT_EQ(batch2.m_callRequests.size(), 1);
        ASSERT_TRUE(batch2.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch3 = batchesManager->nextBatch();
        ASSERT_EQ(batch3.m_batchIndex, 3);
        ASSERT_EQ(batch3.m_callRequests.size(), 1);
        ASSERT_TRUE(batch3.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch4 = batchesManager->nextBatch();
        ASSERT_EQ(batch4.m_batchIndex, 4);
        ASSERT_EQ(batch4.m_callRequests.size(), 1);
        ASSERT_TRUE(batch4.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch5 = batchesManager->nextBatch();
        ASSERT_EQ(batch5.m_batchIndex, 5);
        ASSERT_EQ(batch5.m_callRequests.size(), 1);
        ASSERT_TRUE(!batch5.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch6 = batchesManager->nextBatch();
        ASSERT_EQ(batch6.m_batchIndex, 6);
        ASSERT_EQ(batch6.m_callRequests.size(), 2);
        ASSERT_TRUE(batch6.m_callRequests[0]->isManual());
        ASSERT_TRUE(!batch6.m_callRequests[1]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch7 = batchesManager->nextBatch();
        ASSERT_EQ(batch7.m_batchIndex, 7);
        ASSERT_EQ(batch7.m_callRequests.size(), 1);
        ASSERT_TRUE(batch7.m_callRequests[0]->isManual());
        ASSERT_FALSE(batchesManager->hasNextBatch());
    });

    threadManager.execute([&] {
        batchesManager.reset();
    });

    threadManager.stop();
}

TEST(TEST_NAME, VirtualMachineUnavailableTest) {
    // Test procedure:
    // enabledSince = 0         (enabled)
    // addBLockInfo1 - true
    // addCall
    // addBLockInfo2 - true
    // addBLockInfo3 - false
    // addCall
    // addBLockInfo4 - false
    // create contract environment
    srand(time(nullptr));
    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> results = {true, true, false, false};
    std::deque<vm::CallExecutionResult> executionResults;
    for (const auto& result: results) {
        executionResults.push_back(vm::CallExecutionResult{result});
    }
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, executionResults, MAX_VM_DELAY, 4);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    ContractEnvironmentMock contractEnvironmentMock(executorEnvironmentMock, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request

    std::vector<ManualCallRequest> requests;

    const std::vector<int> callsPerBlock = {0, 1, 0, 1};

    for (uint64_t i = 0; i < 4; i++) {
        for (auto j = 0; j < callsPerBlock[i]; j++) {
            std::vector<uint8_t> params;
            ManualCallRequest request(
                    utils::generateRandomByteValue<CallId>(),
                    "",
                    "",
                    52000000,
                    20 * 1024,
                    CallerKey(),
                    i + 1,
                    params,
                    {}
            );
            requests.push_back(request);
        }
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addBlock(1);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlock(2);
        batchesManager->addBlock(3);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlock(4);
    });
    sleep(10);
    threadManager.execute([&] {
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 1);
        ASSERT_EQ(batch1.m_callRequests.size(), 1);
        ASSERT_TRUE(!batch1.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 2);
        ASSERT_EQ(batch2.m_callRequests.size(), 2);
        ASSERT_TRUE(batch2.m_callRequests[0]->isManual());
        ASSERT_TRUE(!batch2.m_callRequests[1]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch3 = batchesManager->nextBatch();
        ASSERT_EQ(batch3.m_batchIndex, 3);
        ASSERT_EQ(batch3.m_callRequests.size(), 1);
        ASSERT_TRUE(batch3.m_callRequests[0]->isManual());
        ASSERT_FALSE(batchesManager->hasNextBatch());
    });

    threadManager.execute([&] {
        batchesManager.reset();
    });

    threadManager.stop();
}

TEST(TEST_NAME, DelayBatchesTest) {
    // Test procedure:
    // enabledSince = 0         (enabled)
    // addBLockInfo1 - true
    // addCall
    // addBLockInfo2 - true
    // addBLockInfo3 - false
    // addCall
    // addBLockInfo4 - false
    // create contract environment
    srand(time(nullptr));
    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> results = {true, true, false, false};
    std::deque<vm::CallExecutionResult> executionResults;
    for (const auto& result: results) {
        executionResults.push_back(vm::CallExecutionResult{result});
    }
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, executionResults, MAX_VM_DELAY, 4);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    ContractEnvironmentMock contractEnvironmentMock(executorEnvironmentMock, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request

    std::vector<ManualCallRequest> requests;

    const std::vector<int> callsPerBlock = {0, 1, 0, 1};

    for (uint64_t i = 0; i < 4; i++) {
        for (auto j = 0; j < callsPerBlock[i]; j++) {
            std::vector<uint8_t> params;
            ManualCallRequest request(
                    utils::generateRandomByteValue<CallId>(),
                    "",
                    "",
                    52000000,
                    20 * 1024,
                    CallerKey(),
                    i + 1,
                    params,
                    {}
            );
            requests.push_back(request);
        }
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addBlock(1);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlock(2);
        batchesManager->addBlock(3);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlock(4);
    });
    sleep(10);
    threadManager.execute([&] {
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 1);
        ASSERT_EQ(batch1.m_callRequests.size(), 1);
        ASSERT_TRUE(!batch1.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        batchesManager->delayBatch(std::move(batch1));
        auto delay1 = batchesManager->nextBatch();
        ASSERT_EQ(delay1.m_batchIndex, 1);
        ASSERT_EQ(delay1.m_callRequests.size(), 1);
        ASSERT_TRUE(!delay1.m_callRequests[0]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 2);
        ASSERT_EQ(batch2.m_callRequests.size(), 2);
        ASSERT_TRUE(batch2.m_callRequests[0]->isManual());
        ASSERT_TRUE(!batch2.m_callRequests[1]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());
        batchesManager->delayBatch(std::move(batch2));
        auto delay2 = batchesManager->nextBatch();
        ASSERT_EQ(delay2.m_batchIndex, 2);
        ASSERT_EQ(delay2.m_callRequests.size(), 2);
        ASSERT_TRUE(delay2.m_callRequests[0]->isManual());
        ASSERT_TRUE(!delay2.m_callRequests[1]->isManual());
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch3 = batchesManager->nextBatch();
        ASSERT_EQ(batch3.m_batchIndex, 3);
        ASSERT_EQ(batch3.m_callRequests.size(), 1);
        ASSERT_TRUE(batch3.m_callRequests[0]->isManual());
        ASSERT_FALSE(batchesManager->hasNextBatch());
        batchesManager->delayBatch(std::move(batch3));
        auto delay3 = batchesManager->nextBatch();
        ASSERT_EQ(delay3.m_batchIndex, 3);
        ASSERT_EQ(delay3.m_callRequests.size(), 1);
        ASSERT_TRUE(delay3.m_callRequests[0]->isManual());
        ASSERT_FALSE(batchesManager->hasNextBatch());
    });

    threadManager.execute([&] {
        batchesManager.reset();
    });

    threadManager.stop();
}
}