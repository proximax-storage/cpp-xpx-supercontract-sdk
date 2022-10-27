/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <supercontract/DefaultBatchesManager.h>
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

class MessengerMock : public Messenger {
public:
    void sendMessage(const ExecutorKey& key, const std::string& msg) override {};
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
    MessengerMock m_messengerMock;
    std::weak_ptr<storage::Storage> m_storage;
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

    Messenger& messenger() override { return m_messengerMock; }

    std::weak_ptr<storage::Storage> storage() override { return m_storage; }

    ExecutorEventHandler& executorEventHandler() override { return m_executorEventHandlerMock; }

    std::weak_ptr<vm::VirtualMachine> virtualMachine() override { return m_virtualMachineMock; }

    ExecutorConfig& executorConfig() override { return m_executorConfig; }

    boost::asio::ssl::context& sslContext() override { return m_sslContext; }

    ThreadManager& threadManager() override { return m_threadManager; }

    logging::Logger& logger() override { return m_logger; }

};

// create block
BlockHash hash1 = BlockHash();
BlockHash hash2 = BlockHash();
BlockHash hash3 = BlockHash();
BlockHash hash4 = BlockHash();
Block block1 = {
        hash1,
        20
};
Block block2 = {
        hash2,
        21
};
Block block3 = {
        hash3,
        22
};
Block block4 = {
        hash4,
        23
};

// create call requests
std::vector<uint8_t> params;
CallRequestParameters callRequest({ContractKey(),
                                   CallId(),
                                   "",
                                   "",
                                   params,
                                   52000000,
                                   20 * 1024,
                                   CallReferenceInfo{
                                           {},
                                           0,
                                           BlockHash(),
                                           0,
                                           0,
                                           {}
                                   }
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

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });
    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addBlockInfo(block1);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addBlockInfo(block2);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addBlockInfo(block3);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addBlockInfo(block4);
    });
    sleep(1);
    std::promise<void> barrier;
    threadManager.execute([&] {
        batchesManager->nextBatch();
        ASSERT_TRUE(batchesManager->hasNextBatch());
        batchesManager->nextBatch();
        ASSERT_TRUE(batchesManager->hasNextBatch());
        batchesManager->nextBatch();
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

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });
    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addBlockInfo(block1);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addBlockInfo(block2);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addBlockInfo(block3);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addBlockInfo(block4);
    });
    sleep(1);
    std::promise<void> barrier;
    threadManager.execute([&] {
        batchesManager->nextBatch();
        ASSERT_TRUE(batchesManager->hasNextBatch());
        batchesManager->nextBatch();
        ASSERT_TRUE(batchesManager->hasNextBatch());
        batchesManager->nextBatch();
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
    // nextBatchIndex = 0
    // addCall
    // addCall
    // addBLockInfo - false  (batch1) storageSynchronised
    // addBlockInfo - true   (batch2)
    // addCall
    // addCall
    // addBlockInfo - true  (batch3)
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
    std::deque<bool> result = {true, false, true, true, false}; //first element is dummy data
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 0;
    std::unique_ptr<BaseBatchesManager> batchesManager;

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });
    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addBlockInfo(block1);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addBlockInfo(block2);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addBlockInfo(block3);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addBlockInfo(block4);
    });
    sleep(1);
    std::promise<void> barrier;
    threadManager.execute([&] {
        batchesManager->nextBatch();
        ASSERT_TRUE(batchesManager->hasNextBatch());
        batchesManager->nextBatch();
        ASSERT_FALSE(batchesManager->hasNextBatch());
        barrier.set_value();
    });

    barrier.get_future().wait();

    threadManager.execute([&] {
        batchesManager.reset();
    });

    threadManager.stop();
}

TEST(TEST_NAME, StorageSynchronisedFirstBlockFalseTest) {
    // Test procedure:
    // nextBatchIndex = 0
    // addBLockInfo - false
    // addBLockInfo - true  (batch1) storageSynchronised
    // addBLockInfo - true  (batch2)
    // addCall
    // addCall
    // addCall
    // addCall
    // addBlockInfo - false (batch3)
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
    std::deque<bool> result = {true, false, true, true, false}; //first element is dummy data
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager);

    // create default batches manager
    uint64_t index = 0;

    std::unique_ptr<BaseBatchesManager> batchesManager;

    threadManager.execute([&] {
        batchesManager = std::make_unique<DefaultBatchesManager>(index, contractEnvironmentMock,
                                                                 executorEnvironmentMock);
    });

    threadManager.execute([&] {
        batchesManager->setAutomaticExecutionsEnabledSince(0);
        batchesManager->addBlockInfo(block1);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addBlockInfo(block2);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addBlockInfo(block3);
    });
    sleep(1);
    threadManager.execute([&] {
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addManualCall(callRequest);
        batchesManager->addBlockInfo(block4);
    });
    sleep(1);
    std::promise<void> barrier;
    threadManager.execute([&] {
        batchesManager->nextBatch();
        ASSERT_TRUE(batchesManager->hasNextBatch());
        batchesManager->nextBatch();
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