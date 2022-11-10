/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <supercontract/DefaultBatchesManager.h>
#include "ContractEnvironmentMock.h"
#include "ExecutorEnvironmentMock.h"
#include "VirtualMachineMock.h"

#include "utils/Random.h"
#include "gtest/gtest.h"

namespace sirius::contract::test {
#define TEST_NAME DefaultBatchesManagerTest

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

    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> result = {true, true, false, false};
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result, false);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 1;
    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request
    std::vector<Block> blocks;
    std::vector<CallRequestParameters> requests;

    for(uint64_t i=1; i<=4; i++){
        Block block = {
                utils::generateRandomByteValue<BlockHash>(),
                i
        };
        blocks.push_back(block);
    }

    for(auto i=1; i<=4; i++){
        std::vector<uint8_t> params;
        CallRequestParameters request = {
                utils::generateRandomByteValue<ContractKey>(),
                utils::generateRandomByteValue<CallId>(),
                "",
                "",
                params,
                52000000,
                20 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        utils::generateRandomByteValue<BlockHash>(),
                        0,
                        0,
                        {}
                }
        };
        requests.push_back(request);
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlockInfo(blocks[0]);
        batchesManager->addBlockInfo(blocks[1]);
        batchesManager->addBlockInfo(blocks[2]);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlockInfo(blocks[3]);
    });
    sleep(3);
    std::promise<void> barrier;
    threadManager.execute([&] {
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 1);
        ASSERT_EQ(batch1.m_callRequests.size(), 3);
        ASSERT_EQ(batch1.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch1.m_callRequests[1].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch1.m_callRequests[2].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 2);
        ASSERT_EQ(batch2.m_callRequests.size(), 1);
        ASSERT_EQ(batch2.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch3 = batchesManager->nextBatch();
        ASSERT_EQ(batch3.m_batchIndex, 3);
        ASSERT_EQ(batch3.m_callRequests.size(), 2);
        ASSERT_EQ(batch3.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch3.m_callRequests[1].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_FALSE(batchesManager->hasNextBatch());

        barrier.set_value();
    });

    barrier.get_future().wait();

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

    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> result = {false, false, false, false};
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result, false);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 1;
    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request
    std::vector<Block> blocks;
    std::vector<CallRequestParameters> requests;

    for(uint64_t i=1; i<=4; i++){
        Block block = {
                utils::generateRandomByteValue<BlockHash>(),
                i
        };
        blocks.push_back(block);
    }

    for(auto i=1; i<=13; i++){
        std::vector<uint8_t> params;
        CallRequestParameters request = {
                utils::generateRandomByteValue<ContractKey>(),
                utils::generateRandomByteValue<CallId>(),
                "",
                "",
                params,
                52000000,
                20 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        utils::generateRandomByteValue<BlockHash>(),
                        0,
                        0,
                        {}
                }
        };
        requests.push_back(request);
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
        batchesManager->addBlockInfo(blocks[0]);
        batchesManager->addManualCall(requests[8]);
        batchesManager->addManualCall(requests[9]);
        batchesManager->addManualCall(requests[10]);
        batchesManager->addManualCall(requests[11]);
        batchesManager->addBlockInfo(blocks[1]);
        batchesManager->addManualCall(requests[12]);
        batchesManager->addManualCall(requests[13]);
        batchesManager->addBlockInfo(blocks[2]);
        batchesManager->addBlockInfo(blocks[3]);
    });
    sleep(3);
    std::promise<void> barrier;
    threadManager.execute([&] {
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 1);
        ASSERT_EQ(batch1.m_callRequests.size(), 8);
        ASSERT_EQ(batch1.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch1.m_callRequests[1].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch1.m_callRequests[2].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch1.m_callRequests[3].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch1.m_callRequests[4].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch1.m_callRequests[5].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch1.m_callRequests[6].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch1.m_callRequests[7].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 2);
        ASSERT_EQ(batch2.m_callRequests.size(), 4);
        ASSERT_EQ(batch2.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch2.m_callRequests[1].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch2.m_callRequests[2].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch2.m_callRequests[3].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch3 = batchesManager->nextBatch();
        ASSERT_EQ(batch3.m_batchIndex, 3);
        ASSERT_EQ(batch3.m_callRequests.size(), 2);
        ASSERT_EQ(batch3.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch3.m_callRequests[1].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_FALSE(batchesManager->hasNextBatch());
        barrier.set_value();
    });

    barrier.get_future().wait();

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

    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> result = {true, false, true, false};
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result, false);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 1;
    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request
    std::vector<Block> blocks;
    std::vector<CallRequestParameters> requests;

    for(uint64_t i=1; i<=4; i++){
        Block block = {
                utils::generateRandomByteValue<BlockHash>(),
                i
        };
        blocks.push_back(block);
    }

    for(auto i=1; i<=4; i++){
        std::vector<uint8_t> params;
        CallRequestParameters request = {
                utils::generateRandomByteValue<ContractKey>(),
                utils::generateRandomByteValue<CallId>(),
                "",
                "",
                params,
                52000000,
                20 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        utils::generateRandomByteValue<BlockHash>(),
                        0,
                        0,
                        {}
                }
        };
        requests.push_back(request);
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });
    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->onStorageSynchronized(2);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlockInfo(blocks[0]);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlockInfo(blocks[1]);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlockInfo(blocks[2]);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlockInfo(blocks[3]);
    });
    sleep(3);
    std::promise<void> barrier;
    threadManager.execute([&] {
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 3);
        ASSERT_EQ(batch1.m_callRequests.size(), 2);
        ASSERT_EQ(batch1.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch1.m_callRequests[1].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 4);
        ASSERT_EQ(batch2.m_callRequests.size(), 1);
        ASSERT_EQ(batch2.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_FALSE(batchesManager->hasNextBatch());
        barrier.set_value();
    });

    barrier.get_future().wait();

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

    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> result = {true, false, true, false};
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result, false);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request
    std::vector<Block> blocks;
    std::vector<CallRequestParameters> requests;

    for(uint64_t i=1; i<=4; i++){
        Block block = {
                utils::generateRandomByteValue<BlockHash>(),
                i
        };
        blocks.push_back(block);
    }

    for(auto i=1; i<=4; i++){
        std::vector<uint8_t> params;
        CallRequestParameters request = {
                utils::generateRandomByteValue<ContractKey>(),
                utils::generateRandomByteValue<CallId>(),
                "",
                "",
                params,
                52000000,
                20 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        utils::generateRandomByteValue<BlockHash>(),
                        0,
                        0,
                        {}
                }
        };
        requests.push_back(request);
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlockInfo(blocks[0]);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlockInfo(blocks[1]);
        batchesManager->onStorageSynchronized(2);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlockInfo(blocks[2]);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlockInfo(blocks[3]);
    });
    sleep(3);
    std::promise<void> barrier;
    threadManager.execute([&] {
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 3);
        ASSERT_EQ(batch1.m_callRequests.size(), 2);
        ASSERT_EQ(batch1.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch1.m_callRequests[1].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 4);
        ASSERT_EQ(batch2.m_callRequests.size(), 1);
        ASSERT_EQ(batch2.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_FALSE(batchesManager->hasNextBatch());
        barrier.set_value();
    });

    barrier.get_future().wait();

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

    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> result = {true, false, true, false};
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result, false);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request
    std::vector<Block> blocks;
    std::vector<CallRequestParameters> requests;

    for(uint64_t i=1; i<=4; i++){
        Block block = {
                utils::generateRandomByteValue<BlockHash>(),
                i
        };
        blocks.push_back(block);
    }

    for(auto i=1; i<=4; i++){
        std::vector<uint8_t> params;
        CallRequestParameters request = {
                utils::generateRandomByteValue<ContractKey>(),
                utils::generateRandomByteValue<CallId>(),
                "",
                "",
                params,
                52000000,
                20 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        utils::generateRandomByteValue<BlockHash>(),
                        0,
                        0,
                        {}
                }
        };
        requests.push_back(request);
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlockInfo(blocks[0]);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlockInfo(blocks[1]);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlockInfo(blocks[2]);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlockInfo(blocks[3]);
    });
    sleep(3);
    std::promise<void> barrier;
    threadManager.execute([&] {
        batchesManager->onStorageSynchronized(2);
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 3);
        ASSERT_EQ(batch1.m_callRequests.size(), 2);
        ASSERT_EQ(batch1.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch1.m_callRequests[1].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 4);
        ASSERT_EQ(batch2.m_callRequests.size(), 1);
        ASSERT_EQ(batch2.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_FALSE(batchesManager->hasNextBatch());
        barrier.set_value();
    });

    barrier.get_future().wait();

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

    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> result = {true, false, true, false};
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result, false);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request
    std::vector<Block> blocks;
    std::vector<CallRequestParameters> requests;

    for(uint64_t i=1; i<=4; i++){
        Block block = {
                utils::generateRandomByteValue<BlockHash>(),
                i
        };
        blocks.push_back(block);
    }

    for(auto i=1; i<=4; i++){
        std::vector<uint8_t> params;
        CallRequestParameters request = {
                utils::generateRandomByteValue<ContractKey>(),
                utils::generateRandomByteValue<CallId>(),
                "",
                "",
                params,
                52000000,
                20 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        utils::generateRandomByteValue<BlockHash>(),
                        0,
                        0,
                        {}
                }
        };
        requests.push_back(request);
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlockInfo(blocks[0]);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlockInfo(blocks[1]);
        batchesManager->setAutomaticExecutionsEnabledSince(std::nullopt);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlockInfo(blocks[2]);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlockInfo(blocks[3]);
    });
    sleep(3);
    std::promise<void> barrier;
    threadManager.execute([&] {
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 1);
        ASSERT_EQ(batch1.m_callRequests.size(), 1);
        ASSERT_EQ(batch1.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 2);
        ASSERT_EQ(batch2.m_callRequests.size(), 1);
        ASSERT_EQ(batch2.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch3 = batchesManager->nextBatch();
        ASSERT_EQ(batch3.m_batchIndex, 3);
        ASSERT_EQ(batch3.m_callRequests.size(), 1);
        ASSERT_EQ(batch3.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_TRUE(batchesManager->hasNextBatch());

        auto batch4 = batchesManager->nextBatch();
        ASSERT_EQ(batch4.m_batchIndex, 4);
        ASSERT_EQ(batch4.m_callRequests.size(), 1);
        ASSERT_EQ(batch4.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_FALSE(batchesManager->hasNextBatch());
        barrier.set_value();
    });

    barrier.get_future().wait();

    threadManager.execute([&] {
        batchesManager.reset();
    });

    threadManager.stop();
}

// *** Current development is not possible to run two enableSince(true) in a row
// *** This feature will add in the future
//TEST(TEST_NAME, MultipleEnabledSinceTest) {
//    // Test procedure:
//    // enabledSince = 0 (enabled)
//    // addBLockInfo1 - true
//    // addBLockInfo2 - false
//    // addCall
//    // addBLockInfo3 - true
//    // addCall
//    // addBlockInfo4 - false
//    // enabledSince = 9 (disabled at addBlock 5-8, enabled at addBlock 9-12
//    // addBlockInfo5 - true
//    // addBlockInfo6 - false
//    // addCall
//    // addBlockInfo7 - true
//    // addCall
//    // addBlockInfo8 - false
//    // addBlockInfo9  - true
//    // addBlockInfo10 - false
//    // addCall
//    // addBlockInfo11 - true
//    // addCall
//    // addBlockInfo12 - false
//    // create contract environment
//    srand(time(nullptr));
//    ContractKey contractKey;
//    uint64_t automaticExecutionsSCLimit = 0;
//    uint64_t automaticExecutionsSMLimit = 0;
//    DriveKey driveKey;
//    std::set<ExecutorKey> executors;
//
//    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExecutionsSCLimit,
//                                                    automaticExecutionsSMLimit);
//
//    // create executor environment
//    crypto::PrivateKey privateKey;
//    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
//    ExecutorConfig executorConfig;
//    ThreadManager threadManager;
//    std::deque<bool> result = {true, false, true, false, true, false, true, false, true, false, true, false};
//    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result, false);
//    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;
//
//    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
//                                                    threadManager);
//
//    // create default batches manager
//    uint64_t index = 1;
//
//    std::unique_ptr<BaseBatchesManager> batchesManager;
//
//    //create block and request
//    std::vector<Block> blocks;
//    std::vector<CallRequestParameters> requests;
//
//    for(uint64_t i=1; i<=12; i++){
//        Block block = {
//                utils::generateRandomByteValue<BlockHash>(),
//                i
//        };
//        blocks.push_back(block);
//    }
//
//    for(auto i=1; i<=6; i++){
//        std::vector<uint8_t> params;
//        CallRequestParameters request = {
//                utils::generateRandomByteValue<ContractKey>(),
//                utils::generateRandomByteValue<CallId>(),
//                "",
//                "",
//                params,
//                52000000,
//                20 * 1024,
//                CallReferenceInfo{
//                        {},
//                        0,
//                        utils::generateRandomByteValue<BlockHash>(),
//                        0,
//                        0,
//                        {}
//                }
//        };
//        requests.push_back(request);
//    }
//
//    threadManager.execute([&] {
//        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
//                                                                 executorEnvironmentMock);
//    });
//
//    threadManager.execute([&] {
//        batchesManager->setAutomaticExecutionsEnabledSince(0);
//        batchesManager->addBlockInfo(blocks[0]);
//        batchesManager->addBlockInfo(blocks[1]);
//        batchesManager->addManualCall(requests[0]);
//        batchesManager->addBlockInfo(blocks[2]);
//        batchesManager->addManualCall(requests[1]);
//        batchesManager->addBlockInfo(blocks[3]);
//
//        batchesManager->setAutomaticExecutionsEnabledSince(9);
//        batchesManager->addBlockInfo(blocks[4]);
//        batchesManager->addBlockInfo(blocks[5]);
//        batchesManager->addManualCall(requests[2]);
//        batchesManager->addBlockInfo(blocks[6]);
//        batchesManager->addManualCall(requests[3]);
//        batchesManager->addBlockInfo(blocks[7]);
//
//        batchesManager->addBlockInfo(blocks[8]);
//        batchesManager->addBlockInfo(blocks[9]);
//        batchesManager->addManualCall(requests[4]);
//        batchesManager->addBlockInfo(blocks[10]);
//        batchesManager->addManualCall(requests[5]);
//        batchesManager->addBlockInfo(blocks[11]);
//    });
//    sleep(3);
//    std::promise<void> barrier;
//    threadManager.execute([&] {
//        // enabled
//        auto batch1 = batchesManager->nextBatch();
//        ASSERT_EQ(batch1.m_batchIndex, 1);
//        ASSERT_EQ(batch1.m_callRequests.size(), 1);
//        ASSERT_EQ(batch1.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
//        ASSERT_TRUE(batchesManager->hasNextBatch());
//        auto batch2 = batchesManager->nextBatch();
//        ASSERT_EQ(batch2.m_batchIndex, 2);
//        ASSERT_EQ(batch2.m_callRequests.size(), 2);
//        ASSERT_EQ(batch2.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
//        ASSERT_EQ(batch2.m_callRequests[1].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
//        ASSERT_TRUE(batchesManager->hasNextBatch());
//        auto batch3 = batchesManager->nextBatch();
//        ASSERT_EQ(batch3.m_batchIndex, 3);
//        ASSERT_EQ(batch3.m_callRequests.size(), 1);
//        ASSERT_EQ(batch3.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
//        ASSERT_TRUE(batchesManager->hasNextBatch());
//        // disabled
//        auto batch4 = batchesManager->nextBatch();
//        ASSERT_EQ(batch4.m_batchIndex, 4);
//        ASSERT_EQ(batch4.m_callRequests.size(), 1);
//        ASSERT_EQ(batch4.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
//        ASSERT_TRUE(batchesManager->hasNextBatch());
//        auto batch5 = batchesManager->nextBatch();
//        ASSERT_EQ(batch5.m_batchIndex, 5);
//        ASSERT_EQ(batch5.m_callRequests.size(), 2);
//        ASSERT_EQ(batch5.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
//        ASSERT_EQ(batch5.m_callRequests[1].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
//        ASSERT_TRUE(batchesManager->hasNextBatch());
//        // enabled
//        auto batch6 = batchesManager->nextBatch();
//        ASSERT_EQ(batch6.m_batchIndex, 6);
//        ASSERT_EQ(batch6.m_callRequests.size(), 1);
//        ASSERT_EQ(batch6.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
//        ASSERT_FALSE(batchesManager->hasNextBatch());
//        auto batch7 = batchesManager->nextBatch();
//        ASSERT_EQ(batch7.m_batchIndex, 7);
//        ASSERT_EQ(batch7.m_callRequests.size(), 2);
//        ASSERT_EQ(batch7.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
//        ASSERT_EQ(batch7.m_callRequests[1].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
//        ASSERT_TRUE(batchesManager->hasNextBatch());
//        auto batch8 = batchesManager->nextBatch();
//        ASSERT_EQ(batch8.m_batchIndex, 8);
//        ASSERT_EQ(batch8.m_callRequests.size(), 1);
//        ASSERT_EQ(batch8.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
//        ASSERT_FALSE(batchesManager->hasNextBatch());
//
//        barrier.set_value();
//    });
//
//    barrier.get_future().wait();
//
//    threadManager.execute([&] {
//        batchesManager.reset();
//    });
//
//    threadManager.stop();
//}

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

    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> result = {true, false, true, false, true, false, true, false, true, false, true, false};
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result, false);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request
    std::vector<Block> blocks;
    std::vector<CallRequestParameters> requests;

    for(uint64_t i=1; i<=12; i++){
        Block block = {
                utils::generateRandomByteValue<BlockHash>(),
                i
        };
        blocks.push_back(block);
    }

    for(auto i=1; i<=6; i++){
        std::vector<uint8_t> params;
        CallRequestParameters request = {
                utils::generateRandomByteValue<ContractKey>(),
                utils::generateRandomByteValue<CallId>(),
                "",
                "",
                params,
                52000000,
                20 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        utils::generateRandomByteValue<BlockHash>(),
                        0,
                        0,
                        {}
                }
        };
        requests.push_back(request);
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addBlockInfo(blocks[0]);
        batchesManager->addBlockInfo(blocks[1]);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlockInfo(blocks[2]);
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlockInfo(blocks[3]);
        batchesManager->setAutomaticExecutionsEnabledSince(std::nullopt);
        batchesManager->addBlockInfo(blocks[4]);
        batchesManager->addBlockInfo(blocks[5]);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlockInfo(blocks[6]);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlockInfo(blocks[7]);
        batchesManager->setAutomaticExecutionsEnabledSince(9);
        batchesManager->addBlockInfo(blocks[8]);
        batchesManager->addBlockInfo(blocks[9]);
        batchesManager->addManualCall(requests[4]);
        batchesManager->addBlockInfo(blocks[10]);
        batchesManager->addManualCall(requests[5]);
        batchesManager->addBlockInfo(blocks[11]);
    });
    sleep(3);
    std::promise<void> barrier;
    threadManager.execute([&] {
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 1);
        ASSERT_EQ(batch1.m_callRequests.size(), 0);
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 2);
        ASSERT_EQ(batch2.m_callRequests.size(), 0);
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch3 = batchesManager->nextBatch();
        ASSERT_EQ(batch3.m_batchIndex, 3);
        ASSERT_EQ(batch3.m_callRequests.size(), 1);
        ASSERT_EQ(batch3.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch4 = batchesManager->nextBatch();
        ASSERT_EQ(batch4.m_batchIndex, 4);
        ASSERT_EQ(batch4.m_callRequests.size(), 1);
        ASSERT_EQ(batch4.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch5 = batchesManager->nextBatch();
        ASSERT_EQ(batch5.m_batchIndex, 5);
        ASSERT_EQ(batch5.m_callRequests.size(), 1);
        ASSERT_EQ(batch5.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch6 = batchesManager->nextBatch();
        ASSERT_EQ(batch6.m_batchIndex, 6);
        ASSERT_EQ(batch6.m_callRequests.size(), 1);
        ASSERT_EQ(batch6.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch7 = batchesManager->nextBatch();
        ASSERT_EQ(batch7.m_batchIndex, 7);
        ASSERT_EQ(batch7.m_callRequests.size(), 1);
        ASSERT_EQ(batch7.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch8 = batchesManager->nextBatch();
        ASSERT_EQ(batch8.m_batchIndex, 8);
        ASSERT_EQ(batch8.m_callRequests.size(), 2);
        ASSERT_EQ(batch8.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch8.m_callRequests[1].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch9 = batchesManager->nextBatch();
        ASSERT_EQ(batch9.m_batchIndex, 9);
        ASSERT_EQ(batch9.m_callRequests.size(), 1);
        ASSERT_EQ(batch9.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_FALSE(batchesManager->hasNextBatch());
        barrier.set_value();
    });

    barrier.get_future().wait();

    threadManager.execute([&] {
        batchesManager.reset();
    });

    threadManager.stop();
}

TEST(TEST_NAME, TrueCaseVirtualMachineUnavailableTest) {
    // Test procedure:
    // enabledSince = 0         (enabled)
    // addBLockInfo1 - false
    // addCall
    // addBLockInfo2 - false
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

    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::deque<bool> result = {true, true, true, true, true, true, true, true};
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result, true);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request
    std::vector<Block> blocks;
    std::vector<CallRequestParameters> requests;

    for(uint64_t i=1; i<=12; i++){
        Block block = {
                utils::generateRandomByteValue<BlockHash>(),
                i
        };
        blocks.push_back(block);
    }

    for(auto i=1; i<=6; i++){
        std::vector<uint8_t> params;
        CallRequestParameters request = {
                utils::generateRandomByteValue<ContractKey>(),
                utils::generateRandomByteValue<CallId>(),
                "",
                "",
                params,
                52000000,
                20 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        utils::generateRandomByteValue<BlockHash>(),
                        0,
                        0,
                        {}
                }
        };
        requests.push_back(request);
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addBlockInfo(blocks[0]);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlockInfo(blocks[1]);
    });
    sleep(10);
    std::promise<void> barrier;
    threadManager.execute([&] {
        auto batch1 = batchesManager->nextBatch();
        ASSERT_EQ(batch1.m_batchIndex, 1);
        ASSERT_EQ(batch1.m_callRequests.size(), 1);
        ASSERT_EQ(batch1.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
        ASSERT_TRUE(batchesManager->hasNextBatch());
        auto batch2 = batchesManager->nextBatch();
        ASSERT_EQ(batch2.m_batchIndex, 2);
        ASSERT_EQ(batch2.m_callRequests.size(), 2);
        ASSERT_EQ(batch2.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
        ASSERT_EQ(batch2.m_callRequests[1].m_callLevel, vm::CallRequest::CallLevel::AUTOMATIC);
        ASSERT_FALSE(batchesManager->hasNextBatch());
        barrier.set_value();
    });

    barrier.get_future().wait();

    threadManager.execute([&] {
        batchesManager.reset();
    });

    threadManager.stop();
}

    TEST(TEST_NAME, FalseCaseVirtualMachineUnavailableTest) {
        // Test procedure:
        // enabledSince = 0         (enabled)
        // addBLockInfo1 - false
        // addCall
        // addBLockInfo2 - false
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

        ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExecutionsSCLimit,
                                                        automaticExecutionsSMLimit);

        // create executor environment
        crypto::PrivateKey privateKey;
        crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
        ExecutorConfig executorConfig;
        ThreadManager threadManager;
        std::deque<bool> result = {false, false, false, false, false, false, false, false};
        auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result, true);
        std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

        ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                        threadManager);

        // create default batches manager
        uint64_t index = 1;

        std::unique_ptr<BaseBatchesManager> batchesManager;

        //create block and request
        std::vector<Block> blocks;
        std::vector<CallRequestParameters> requests;

        for(uint64_t i=1; i<=12; i++){
            Block block = {
                    utils::generateRandomByteValue<BlockHash>(),
                    i
            };
            blocks.push_back(block);
        }

        for(auto i=1; i<=6; i++){
            std::vector<uint8_t> params;
            CallRequestParameters request = {
                    utils::generateRandomByteValue<ContractKey>(),
                    utils::generateRandomByteValue<CallId>(),
                    "",
                    "",
                    params,
                    52000000,
                    20 * 1024,
                    CallReferenceInfo{
                            {},
                            0,
                            utils::generateRandomByteValue<BlockHash>(),
                            0,
                            0,
                            {}
                    }
            };
            requests.push_back(request);
        }

        threadManager.execute([&] {
            batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                     executorEnvironmentMock);
        });

        threadManager.execute([&] {
            batchesManager->setAutomaticExecutionsEnabledSince(0);
            batchesManager->addBlockInfo(blocks[0]);
            batchesManager->addManualCall(requests[0]);
            batchesManager->addBlockInfo(blocks[1]);
            batchesManager->addBlockInfo(blocks[2]);
            batchesManager->addManualCall(requests[0]);
            batchesManager->addBlockInfo(blocks[3]);
        });
        sleep(10);
        std::promise<void> barrier;
        threadManager.execute([&] {
            auto batch1 = batchesManager->nextBatch();
            ASSERT_EQ(batch1.m_batchIndex, 1);
            ASSERT_EQ(batch1.m_callRequests.size(), 1);
            ASSERT_EQ(batch1.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
            ASSERT_TRUE(batchesManager->hasNextBatch());
            auto batch2 = batchesManager->nextBatch();
            ASSERT_EQ(batch2.m_batchIndex, 2);
            ASSERT_EQ(batch2.m_callRequests.size(), 1);
            ASSERT_EQ(batch2.m_callRequests[0].m_callLevel, vm::CallRequest::CallLevel::MANUAL);
            ASSERT_FALSE(batchesManager->hasNextBatch());
            barrier.set_value();
        });

        barrier.get_future().wait();

        threadManager.execute([&] {
            batchesManager.reset();
        });

        threadManager.stop();
    }
//    TODO
//    2. add delayBatch test case
}