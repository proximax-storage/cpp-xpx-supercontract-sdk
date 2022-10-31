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

    class ExecutorEventHandlerMock: public ExecutorEventHandler{
    public:
        virtual ~ExecutorEventHandlerMock(){}
        void endBatchTransactionIsReady(const EndBatchExecutionTransactionInfo&) override {};
        void endBatchSingleTransactionIsReady(const EndBatchExecutionSingleTransactionInfo&) override {};
    };

    class ExecutorEnvironmentMock : public ExecutorEnvironment {
    private:
        crypto::KeyPair m_keyPair;
        std::weak_ptr<vm::VirtualMachine> m_virtualMachineMock;
        ExecutorConfig m_executorConfig;
        std::weak_ptr<storage::Storage> m_storage;
        ExecutorEventHandlerMock m_executorEventHandlerMock;
        boost::asio::ssl::context m_sslContext{boost::asio::ssl::context::tlsv12_client};
        ThreadManager m_threadManager;
        logging::Logger m_logger;
        

    public:
        ExecutorEnvironmentMock(crypto::KeyPair&& keyPair,
                                std::weak_ptr<vm::VirtualMachine> virtualMachineMock,
                                ExecutorConfig executorConfig)
            :m_keyPair(std::move(keyPair)),
            m_virtualMachineMock(virtualMachineMock),
            m_executorConfig(executorConfig),
            m_logger(getLoggerConfig(), "executor")
        {}

        virtual ~ExecutorEnvironmentMock(){}

        const crypto::KeyPair& keyPair() const override { return m_keyPair; }

        std::weak_ptr<messenger::Messenger> messenger() override { return {}; }

        std::weak_ptr<storage::Storage> storage() override { return m_storage; }

        ExecutorEventHandler& executorEventHandler() override { return m_executorEventHandlerMock; }

        std::weak_ptr<vm::VirtualMachine> virtualMachine() override { return m_virtualMachineMock; }

        ExecutorConfig& executorConfig() override { return m_executorConfig; }

        boost::asio::ssl::context& sslContext() override { return m_sslContext; }

        ThreadManager& threadManager() { return m_threadManager; }

        logging::Logger& logger() override { return m_logger; }
    
    private:
        logging::LoggerConfig getLoggerConfig() {
            logging::LoggerConfig config;
            config.setLogToConsole(true);
            config.setLogPath({});
            return config;
        }
    };

    class VirtualMachineMock : public vm::VirtualMachine {
    private: 
        ThreadManager m_mainThreadManager;
        ThreadManager m_threadManager;
        std::map<CallId, Timer> timers;
    public:
    
        virtual ~VirtualMachineMock(){}

        void executeCall(const CallRequest& request,
                         std::weak_ptr<vm::VirtualMachineInternetQueryHandler> internetQueryHandler,
                         std::weak_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                         std::shared_ptr<AsyncQueryCallback<vm::CallExecutionResult>> callback) {
            srand(time(0));
            sleep(rand()%10);
        }
    };

#define TEST_NAME Example

    TEST(TEST_NAME, Example) {
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
        std::weak_ptr<VirtualMachineMock> virtualMachineMock = std::make_shared<VirtualMachineMock>();
        ExecutorConfig executorConfig;

        ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), virtualMachineMock, executorConfig);

        // create default batches manager
        uint64_t index = 1;
        DefaultBatchesManager defaultBatchesManager(index, contractEnvironmentMock, executorEnvironmentMock);

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

        Block block1 = {
            BlockHash(),
            10001
        };
        Block block2 = {
            BlockHash(),
            10002
        };
        Block block3 = {
            BlockHash(),
            10003
        };
        Block block4 = {
            BlockHash(),
            10004
        };

        
        ThreadManager threadManager; 

        defaultBatchesManager.setAutomaticExecutionsEnabledSince(0);
        defaultBatchesManager.addCall(callRequest1);
        // defaultBatchesManager.addCall(callRequest2);
        // defaultBatchesManager.addBlockInfo(block1); //true
        // defaultBatchesManager.addBlockInfo(block2); //true 
        // defaultBatchesManager.addBlockInfo(block3); //false
        // defaultBatchesManager.addCall(callRequest3);
        // defaultBatchesManager.addCall(callRequest4);
        // defaultBatchesManager.addBlockInfo(block4); //false
        
    }
}