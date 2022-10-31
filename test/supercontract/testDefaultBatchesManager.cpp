/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <supercontract/DefaultBatchesManager.h>
#include "utils/Random.h"
#include "gtest/gtest.h"

namespace sirius::contract::test {
class ContractEnvironmentMock : public ContractEnvironment {
private:
    ContractKey m_contractKey;
    DriveKey m_driveKey;
    std::set<ExecutorKey> m_executors;
    uint64_t m_automaticExecutionsSCLimit;
    uint64_t m_automaticExecutionsSMLimit;
    ContractConfig m_contractConfig;

public:
    ContractEnvironmentMock(ContractKey& contractKey,
                            uint64_t automaticExecutionsSCLimit,
                            uint64_t automaticExecutionsSMLimit)
            : m_contractKey(contractKey), m_automaticExecutionsSCLimit(automaticExecutionsSCLimit)
              , m_automaticExecutionsSMLimit(automaticExecutionsSMLimit) {}

    const ContractKey& contractKey() const override { return m_contractKey; }

    const DriveKey& driveKey() const override { return m_driveKey; }

    const std::set<ExecutorKey>& executors() const override { return m_executors; }

    uint64_t automaticExecutionsSCLimit() const override { return m_automaticExecutionsSCLimit; }

    uint64_t automaticExecutionsSMLimit() const override { return m_automaticExecutionsSMLimit; }

    const ContractConfig& contractConfig() const override { return m_contractConfig; }

    void finishTask() override {}

    void addSynchronizationTask() override {}

    void delayBatchExecution(Batch&& batch) override {}
};

class ExecutorEventHandlerMock : public ExecutorEventHandler {
public:
    void endBatchTransactionIsReady(const EndBatchExecutionTransactionInfo&) override {};

    void endBatchSingleTransactionIsReady(const EndBatchExecutionSingleTransactionInfo&) override {};
};

class VirtualMachineMock : public vm::VirtualMachine {
private:
    ThreadManager& m_threadManager;
    std::deque<bool> m_result;
    std::map<CallId, Timer> m_timers;

public:
    VirtualMachineMock(ThreadManager& threadManager, std::deque<bool> result)
            : m_threadManager(threadManager), m_result(result) {}

    void executeCall(const vm::CallRequest& request,
                     std::weak_ptr<vm::VirtualMachineInternetQueryHandler> internetQueryHandler,
                     std::weak_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                     std::shared_ptr<AsyncQueryCallback<vm::CallExecutionResult>> callback) {

        m_result.pop_front();
        m_timers[request.m_callId] = m_threadManager.startTimer(rand() % 1000, [=, this]() mutable {
            vm::CallExecutionResult result{
                    m_result.front(),
                    0,
                    0,
                    0,
            };
            callback->postReply(result);
        });
    }
};

class ExecutorEnvironmentMock : public ExecutorEnvironment {
private:
    crypto::KeyPair m_keyPair;
    std::weak_ptr<VirtualMachineMock> m_virtualMachineMock;
    ExecutorConfig m_executorConfig;
    std::weak_ptr<storage::StorageModifier> m_storage;
    ExecutorEventHandlerMock m_executorEventHandlerMock;
    boost::asio::ssl::context m_sslContext{boost::asio::ssl::context::tlsv12_client};
    ThreadManager& m_threadManager;
    logging::Logger m_logger;


public:
    ExecutorEnvironmentMock(crypto::KeyPair&& keyPair,
                            std::weak_ptr<VirtualMachineMock> virtualMachineMock,
                            ExecutorConfig executorConfig,
                            ThreadManager& threadManager)
            : m_keyPair(std::move(keyPair)), m_virtualMachineMock(virtualMachineMock), m_executorConfig(executorConfig)
              , m_threadManager(threadManager), m_logger(executorConfig.loggerConfig(), "executor") {}

    const crypto::KeyPair& keyPair() const override { return m_keyPair; }

    std::weak_ptr<messenger::Messenger> messenger() override {
        return {};
    }

    std::weak_ptr<storage::StorageModifier> storageModifier() override { return m_storage; }

    ExecutorEventHandler& executorEventHandler() override { return m_executorEventHandlerMock; }

    std::weak_ptr<vm::VirtualMachine> virtualMachine() override { return m_virtualMachineMock; }

    ExecutorConfig& executorConfig() override { return m_executorConfig; }

    boost::asio::ssl::context& sslContext() override { return m_sslContext; }

    ThreadManager& threadManager() override { return m_threadManager; }

    logging::Logger& logger() override { return m_logger; }

};

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
    uint64_t automaticExucutionsSCLimit = 0;
    uint64_t automaticExucutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;
    ContractConfig contractConfig;

    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExucutionsSCLimit,
                                                    automaticExucutionsSMLimit);

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
    uint64_t automaticExucutionsSCLimit = 0;
    uint64_t automaticExucutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;
    ContractConfig contractConfig;

    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExucutionsSCLimit,
                                                    automaticExucutionsSMLimit);

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
    uint64_t automaticExucutionsSCLimit = 0;
    uint64_t automaticExucutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;
    ContractConfig contractConfig;

    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExucutionsSCLimit,
                                                    automaticExucutionsSMLimit);

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
    uint64_t automaticExucutionsSCLimit = 0;
    uint64_t automaticExucutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;
    ContractConfig contractConfig;

    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExucutionsSCLimit,
                                                    automaticExucutionsSMLimit);

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
    uint64_t automaticExucutionsSCLimit = 0;
    uint64_t automaticExucutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;
    ContractConfig contractConfig;

    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExucutionsSCLimit,
                                                    automaticExucutionsSMLimit);

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
    // addcall
    // addBLockInfo(height 20) - true
    // addcall
    // addBLockInfo(height 21) - false
    // enabledSince = nullopt (disabled)
    // addcall
    // addBLockInfo(height 22) - true
    // addcall
    // addBlockInfo(height 23) - false
    // create contract environment
    ContractKey contractKey;
    uint64_t automaticExucutionsSCLimit = 0;
    uint64_t automaticExucutionsSMLimit = 0;
    DriveKey driveKey;
    std::set<ExecutorKey> executors;
    ContractConfig contractConfig;

    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExucutionsSCLimit,
                                                    automaticExucutionsSMLimit);

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