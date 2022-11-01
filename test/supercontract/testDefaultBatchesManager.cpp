/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <supercontract/DefaultBatchesManager.h>
#include "TestUtils.h"

#include <utility>
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
    std::deque<bool> result = {true, true, true, false, false}; //first element is dummy data
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 1;
    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request
    std::vector<Block> blocks;
    std::vector<vm::CallRequest> requests;

    for(auto i=1; i<=4; i++){
        uint64_t height = 0;
        Block block = {
            utils::generateRandomByteValue<BlockHash>(),
            ++height
        };
        blocks.push_back(block);
    }

    for(auto i=1; i<=4; i++){
        std::vector<uint8_t> params;
        vm::CallRequest callRequest({utils::generateRandomByteValue<ContractKey>(),
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
                                },
                                vm::CallRequest::CallLevel::MANUAL);
        requests.push_back(callRequest);
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
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addBlockInfo(blocks[1]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addBlockInfo(blocks[2]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(requests[2]);
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlockInfo(blocks[3]);
    });
    sleep(1);
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
    std::deque<bool> result = {true, false, false, false, false}; //first element is dummy data
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 1;
    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request
    std::vector<Block> blocks;
    std::vector<vm::CallRequest> requests;

    for(auto i=1; i<=4; i++){
        uint64_t height = 0;
        Block block = {
            utils::generateRandomByteValue<BlockHash>(),
            ++height
        };
        blocks.push_back(block);
    }

    for(auto i=1; i<=13; i++){
        std::vector<uint8_t> params;
        vm::CallRequest callRequest({utils::generateRandomByteValue<ContractKey>(),
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
                                },
                                vm::CallRequest::CallLevel::MANUAL);
        requests.push_back(callRequest);
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
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(requests[8]);
        batchesManager->addManualCall(requests[9]);
        batchesManager->addManualCall(requests[10]);
        batchesManager->addManualCall(requests[11]);
        batchesManager->addBlockInfo(blocks[1]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(requests[12]);
        batchesManager->addManualCall(requests[13]);
        batchesManager->addBlockInfo(blocks[2]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addBlockInfo(blocks[3]);
    });
    sleep(1);
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
    std::deque<bool> result = {true, true, false, true, false}; //first element is dummy data
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 1;
    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request
    std::vector<Block> blocks;
    std::vector<vm::CallRequest> requests;

    for(auto i=1; i<=4; i++){
        uint64_t height = 0;
        Block block = {
            utils::generateRandomByteValue<BlockHash>(),
            ++height
        };
        blocks.push_back(block);
    }

    for(auto i=1; i<=4; i++){
        std::vector<uint8_t> params;
        vm::CallRequest callRequest({utils::generateRandomByteValue<ContractKey>(),
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
                                },
                                vm::CallRequest::CallLevel::MANUAL);
        requests.push_back(callRequest);
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
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlockInfo(blocks[1]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlockInfo(blocks[2]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlockInfo(blocks[3]);
    });
    sleep(1);
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
    std::deque<bool> result = {true, true, false, true, false}; //first element is dummy data
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request
    std::vector<Block> blocks;
    std::vector<vm::CallRequest> requests;

    for(auto i=1; i<=4; i++){
        uint64_t height = 0;
        Block block = {
            utils::generateRandomByteValue<BlockHash>(),
            ++height
        };
        blocks.push_back(block);
    }

    for(auto i=1; i<=4; i++){
        std::vector<uint8_t> params;
        vm::CallRequest callRequest({utils::generateRandomByteValue<ContractKey>(),
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
                                },
                                vm::CallRequest::CallLevel::MANUAL);
        requests.push_back(callRequest);
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlockInfo(blocks[0]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlockInfo(blocks[1]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->onStorageSynchronized(2);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlockInfo(blocks[2]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlockInfo(blocks[3]);
    });
    sleep(1);
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
    std::deque<bool> result = {true, true, false, true, false}; //first element is dummy data
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request
    std::vector<Block> blocks;
    std::vector<vm::CallRequest> requests;

    for(auto i=1; i<=4; i++){
        uint64_t height = 0;
        Block block = {
            utils::generateRandomByteValue<BlockHash>(),
            ++height
        };
        blocks.push_back(block);
    }

    for(auto i=1; i<=4; i++){
        std::vector<uint8_t> params;
        vm::CallRequest callRequest({utils::generateRandomByteValue<ContractKey>(),
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
                                },
                                vm::CallRequest::CallLevel::MANUAL);
        requests.push_back(callRequest);
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlockInfo(blocks[0]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlockInfo(blocks[1]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlockInfo(blocks[2]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlockInfo(blocks[3]);
    });
    sleep(1);
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
    // addBLockInfo(height 20) - true
    // addCall
    // addBLockInfo(height 21) - false
    // enabledSince = null opt (disabled)
    // addCall
    // addBLockInfo(height 22) - true
    // addCall
    // addBlockInfo(height 23) - false
    // create contract environment
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
    std::deque<bool> result = {true, true, false, true, false}; //first element is dummy data
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 1;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    //create block and request
    std::vector<Block> blocks;
    std::vector<vm::CallRequest> requests;

    for(auto i=1; i<=4; i++){
        uint64_t height = 0;
        Block block = {
            utils::generateRandomByteValue<BlockHash>(),
            ++height
        };
        blocks.push_back(block);
    }

    for(auto i=1; i<=4; i++){
        std::vector<uint8_t> params;
        vm::CallRequest callRequest({utils::generateRandomByteValue<ContractKey>(),
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
                                },
                                vm::CallRequest::CallLevel::MANUAL);
        requests.push_back(callRequest);
    }

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(requests[0]);
        batchesManager->addBlockInfo(blocks[0]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(requests[1]);
        batchesManager->addBlockInfo(blocks[1]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(std::nullopt);
        batchesManager->addManualCall(requests[2]);
        batchesManager->addBlockInfo(blocks[2]);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(requests[3]);
        batchesManager->addBlockInfo(blocks[3]);
    });
    sleep(1);
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
}