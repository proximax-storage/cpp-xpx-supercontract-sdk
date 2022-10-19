/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "gtest/gtest.h"
#include "supercontract/DefaultBatchesManager.h"

namespace sirius::contract::test {
    class ContractEnvironmentMock: public ContractEnvironment {
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
            :m_contractKey(contractKey),
            m_automaticExecutionsSCLimit(automaticExecutionsSCLimit),
            m_automaticExecutionsSMLimit(automaticExecutionsSMLimit)
        {}

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
    
    class MessengerMock: public Messenger {
    public:
        void sendMessage(const ExecutorKey& key, const std::string& msg) override {};
    };

    class ExecutorEventHandlerMock: public ExecutorEventHandler{
    public:
        void endBatchTransactionIsReady(const EndBatchExecutionTransactionInfo&) override {};
        void endBatchSingleTransactionIsReady(const EndBatchExecutionSingleTransactionInfo&) override {};
    };

    class VirtualMachineMock : public vm::VirtualMachine {
    private:
        std::shared_ptr<ThreadManager> m_threadManager;
        std::map<CallId, Timer> m_timers;
    public:
        VirtualMachineMock(std::shared_ptr<ThreadManager> threadManager)
            :m_threadManager(threadManager)
            {}
        void executeCall(const CallRequest& request,
                         std::weak_ptr<vm::VirtualMachineInternetQueryHandler> internetQueryHandler,
                         std::weak_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                         std::shared_ptr<AsyncQueryCallback<vm::CallExecutionResult>> callback) {
            
            // srand(time(0));
            // std::cout << m_threadManager.get()->threadId() << std::endl;
            m_timers[request.m_callId] = m_threadManager.get()->startTimer((1000), [&callback]{
            std::cout << "vm executing2" << std::endl;
                vm::CallExecutionResult result {
                    true,
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
        std::shared_ptr<ThreadManager> m_pThreadManager;
        logging::Logger m_logger;
        

    public:
        ExecutorEnvironmentMock(crypto::KeyPair&& keyPair,
                                std::weak_ptr<VirtualMachineMock> virtualMachineMock,
                                ExecutorConfig executorConfig,
                                std::shared_ptr<ThreadManager> pThreadManager)
            :m_keyPair(std::move(keyPair)),
            m_virtualMachineMock(virtualMachineMock),
            m_executorConfig(executorConfig),
            m_pThreadManager(pThreadManager),
            m_logger(executorConfig.loggerConfig(), "executor")
        {}

        const crypto::KeyPair& keyPair() const override { return m_keyPair; }

        Messenger& messenger() override { return m_messengerMock; }

        std::weak_ptr<storage::Storage> storage() override { return m_storage; }

        ExecutorEventHandler& executorEventHandler() override { return m_executorEventHandlerMock; }

        std::weak_ptr<vm::VirtualMachine> virtualMachine() override { return m_virtualMachineMock; }

        ExecutorConfig& executorConfig() override { return m_executorConfig; }

        boost::asio::ssl::context& sslContext() override { return m_sslContext; }

        ThreadManager& threadManager() override { return *m_pThreadManager; }

        logging::Logger& logger() override { return m_logger; }
    
    };

#define TEST_NAME DefaultBatchesManagerTest

    TEST(TEST_NAME, BatchesTest) {
        // create contract environment
        ContractKey contractKey;
        uint64_t automaticExucutionsSCLimit = 0;
        uint64_t automaticExucutionsSMLimit = 0;
        DriveKey driveKey;
        std::set<ExecutorKey> executors;
        ContractConfig contractConfig;

        ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExucutionsSCLimit, automaticExucutionsSMLimit);

        // create executor environment
        crypto::PrivateKey privateKey;
        crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
        auto threadManager = std::make_shared<ThreadManager>();
        auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager);
        std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;
        ExecutorConfig executorConfig;
        
        ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig, threadManager);

        // create default batches manager
        uint64_t index = 1;
        DefaultBatchesManager defaultBatchesManager(index, contractEnvironmentMock, executorEnvironmentMock);

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
        CallRequest callRequest1 = {
            ContractKey(),
            CallId(),
            "",
            "",
            params,
            52000000,
            20 * 1024,
            CallRequest::CallLevel::AUTOMATIC,
            CallReferenceInfo {
                {},
                0,
                BlockHash(),
                0,
                0,
                {}
            }
        };
        CallRequest callRequest2 = {
            ContractKey(),
            CallId(),
            "",
            "",
            params,
            52000000,
            20 * 1024,
            CallRequest::CallLevel::AUTOMATIC,
            CallReferenceInfo {
                {},
                0,
                BlockHash(),
                0,
                0,
                {}
            }
        };
        CallRequest callRequest3 = {
            ContractKey(),
            CallId(),
            "",
            "",
            params,
            52000000,
            20 * 1024,
            CallRequest::CallLevel::AUTOMATIC,
            CallReferenceInfo {
                {},
                0,
                BlockHash(),
                0,
                0,
                {}
            }
        };
        CallRequest callRequest4 = {
            ContractKey(),
            CallId(),
            "",
            "",
            params,
            52000000,
            20 * 1024,
            CallRequest::CallLevel::AUTOMATIC,
            CallReferenceInfo {
                {},
                0,
                BlockHash(),
                0,
                0,
                {}
            }
        };
        
        defaultBatchesManager.setAutomaticExecutionsEnabledSince(0);
        defaultBatchesManager.addCall(callRequest1); 
        std::cout << "hasNext: " << defaultBatchesManager.hasNextBatch() << std::endl;
        defaultBatchesManager.addCall(callRequest2); 
        defaultBatchesManager.addBlockInfo(block1); // batch 1 (callReq1, callReq2, block1)
        std::cout << "added block, hasNext: " << defaultBatchesManager.hasNextBatch() << std::endl;
        std::cout << "batch size" << defaultBatchesManager.m_batches.size() << std::endl;

        // if(defaultBatchesManager.m_batches.begin()->second.m_batchFormationStatus == DefaultBatchesManager::DraftBatch::BatchFormationStatus::AUTOMATIC){
        //     std::cout << "auto" << std::endl;
        // }
        // if(defaultBatchesManager.m_batches.begin()->second.m_batchFormationStatus == DefaultBatchesManager::DraftBatch::BatchFormationStatus::MANUAL){
        //     std::cout << "manual" << std::endl;
        // }
        // if(defaultBatchesManager.m_batches.begin()->second.m_batchFormationStatus == DefaultBatchesManager::DraftBatch::BatchFormationStatus::FINISHED){
        //     std::cout << "finish" << std::endl;
        // }

        // defaultBatchesManager.addBlockInfo(block2); // batch 2
        // std::cout << "batch size" << defaultBatchesManager.m_batches.size() << std::endl;
        // std::cout << defaultBatchesManager.hasNextBatch() << std::endl;

        // defaultBatchesManager.addBlockInfo(block3); // batch 3
        // std::cout << "batch size" << defaultBatchesManager.m_batches.size() << std::endl;
        // std::cout << defaultBatchesManager.hasNextBatch() << std::endl;

        // defaultBatchesManager.addCall(callRequest3);
        // defaultBatchesManager.addCall(callRequest4);
        // defaultBatchesManager.addBlockInfo(block4); // batch 4 (callReq3, callReq4, block4)
        // std::cout << "batch size" << defaultBatchesManager.m_batches.size() << std::endl;
        // std::cout << defaultBatchesManager.hasNextBatch() << std::endl;
        
        // defaultBatchesManager.nextBatch(); // consume batch 1
        // ASSERT_TRUE(defaultBatchesManager.hasNextBatch());
        
        // defaultBatchesManager.nextBatch(); // consume batch 2
        // ASSERT_TRUE(defaultBatchesManager.hasNextBatch());

        // defaultBatchesManager.nextBatch(); // consume batch 3
        // ASSERT_TRUE(defaultBatchesManager.hasNextBatch());

        // defaultBatchesManager.nextBatch(); // consume batch 4
        // ASSERT_FALSE(defaultBatchesManager.hasNextBatch());
    }
}