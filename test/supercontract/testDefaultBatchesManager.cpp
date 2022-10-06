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
        ContractEnvironmentMock(ContractKey contractKey,
                                uint64_t automaticExecutionsSCLimit,
                                uint64_t automaticExecutionsSMLimit)
            : m_contractKey(contractKey),
              m_automaticExecutionsSCLimit(automaticExecutionsSCLimit),
              m_automaticExecutionsSMLimit(automaticExecutionsSMLimit)
        {}

        ~ContractEnvironmentMock() = default;

        const ContractKey &contractKey() const override {}

        const DriveKey &driveKey() const override {}

        const std::set<ExecutorKey> &executors() const override {}

        uint64_t automaticExecutionsSCLimit() const override {}

        uint64_t automaticExecutionsSMLimit() const override {}

        const ContractConfig &contractConfig() const override {}

        void finishTask() override{};

        void addSynchronizationTask() override{};

        void delayBatchExecution(Batch &&batch) override;
    };

    class ExecutorEnvironmentMock : public ExecutorEnvironment {

    private:
        const crypto::KeyPair &m_keyPair;
        std::shared_ptr<storage::Storage> m_storage;
        std::shared_ptr<vm::VirtualMachine> m_virtualMachine;
        ExecutorConfig m_executorConfig;

    public:
        ExecutorEnvironmentMock(crypto::KeyPair &keyPair,
                                std::shared_ptr<storage::Storage> storage,
                                std::shared_ptr<vm::VirtualMachine> virtualMachine,
                                ExecutorConfig executorConfig)
            : m_keyPair(keyPair),
              m_storage(storage),
              m_virtualMachine(virtualMachine),
              m_executorConfig(executorConfig)
        {}

        ~ExecutorEnvironmentMock() = default;

        const crypto::KeyPair &keyPair() const override {}

        Messenger &messenger() override {}

        std::weak_ptr<storage::Storage> storage() override {}

        ExecutorEventHandler &executorEventHandler() override {}

        std::weak_ptr<vm::VirtualMachine> virtualMachine() override {}

        ExecutorConfig &executorConfig() override {}

        boost::asio::ssl::context &sslContext() override {}

        ThreadManager &threadManager() override {
            // return m_threadManager;
        }

        logging::Logger &logger() override {}
    };

    class StorageMock : public storage::Storage {
    private:
    public:
        ~StorageMock() = default;

        void synchronizeStorage(const DriveKey &driveKey, const StorageHash &storageHash,
                                std::shared_ptr<AsyncQueryCallback<bool>> callback) override {}

        void initiateModifications(const DriveKey &driveKey,
                              std::shared_ptr<AsyncQueryCallback<bool>> callback) override {}

        void applySandboxStorageModifications(const DriveKey &driveKey,
                                              bool success,
                                              std::shared_ptr<AsyncQueryCallback<storage::SandboxModificationDigest>> callback) override {}

        void evaluateStorageHash(const DriveKey &driveKey,
                            std::shared_ptr<AsyncQueryCallback<storage::StorageState>> callback) override {}

        void applyStorageModifications(const DriveKey &driveKey, bool success,
                                       std::shared_ptr<AsyncQueryCallback<bool>> callback) override {}

        void getAbsolutePath(const DriveKey &driveKey, const std::string &relativePath,
                        std::shared_ptr<AsyncQueryCallback<std::string>> callback) override {}
    };

    class VirtualMachineMock : public vm::VirtualMachine {
    private:
        std::weak_ptr<DefaultBatchesManager> m_defaultBatchesManager;
        ThreadManager m_mainThreadManager;
        ThreadManager m_threadManager;
        std::map<CallId, Timer> m_timers;
        std::deque<bool> m_executionResult;

    public:
        VirtualMachineMock(std::weak_ptr<DefaultBatchesManager> defaultBatchesManager)
            :m_defaultBatchesManager(defaultBatchesManager),
            m_executionResult({true, true, false, false})
            {}
    
        ~VirtualMachineMock() = default;

        void executeCall(const CallRequest& request,
                         std::weak_ptr<vm::VirtualMachineInternetQueryHandler> internetQueryHandler,
                         std::weak_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                         std::shared_ptr<AsyncQueryCallback<vm::CallExecutionResult>> callback) {
            m_threadManager.execute([=, this] {
                bool result = m_executionResult.front();
                m_executionResult.pop_front();
                m_timers[request.m_callId] = m_threadManager.startTimer(rand() % 10000, [=, this] {
                    m_mainThreadManager.execute([=, this] {
                        vm::CallExecutionResult callExecutionResult= {
                            result,
                            1,
                            1,
                            1
                        };
                        // onSuperContractCallExecuted has change to private and be called from addblock()
                        m_defaultBatchesManager.lock()->onSuperContractCallExecuted(request.m_callId, callExecutionResult);
                    });
                });
            });
        }

        ThreadManager &threadManager() {
            return m_mainThreadManager;
        }
    };

#define TEST_NAME Example

    TEST(TEST_NAME, Example) {
        // create contract environment
        ContractKey contractKey;
        uint64_t automaticExucutionsSCLimit = 0;
        uint64_t automaticExucutionsSMLimit = 0;

        ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExucutionsSCLimit, automaticExucutionsSMLimit);

        // create executor environment
        crypto::PrivateKey privateKey;
        const crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
        std::weak_ptr<DefaultBatchesManager> pDefaultBatchesManager;
        StorageMock storageMock;
        VirtualMachineMock virtualMachineMock(pDefaultBatchesManager);
        ExecutorConfig executorConfig;

        // im not sure why its says does not match arguments of the contructors
        ExecutorEnvironmentMock executorEnvironmentMock(keyPair, storageMock, virtualMachineMock, executorConfig);

        // create default batches manager
        uint64_t index = 1;
        pDefaultBatchesManager = std::make_shared<DefaultBatchesManager>(index, contractEnvironmentMock, executorEnvironmentMock);

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

        pDefaultBatchesManager.lock()->setAutomaticExecutionsEnabledSince(0);
        pDefaultBatchesManager.lock()->addCall(callRequest1);
        pDefaultBatchesManager.lock()->addCall(callRequest2);
        pDefaultBatchesManager.lock()->addBlockInfo(block1);
        pDefaultBatchesManager.lock()->addBlockInfo(block2);
        pDefaultBatchesManager.lock()->addBlockInfo(block3);
        pDefaultBatchesManager.lock()->addCall(callRequest3);
        pDefaultBatchesManager.lock()->addCall(callRequest4);
        pDefaultBatchesManager.lock()->addBlockInfo(block4);
        
        auto &mainThread = virtualMachineMock.threadManager();
        vm::CallExecutionResult callExecutionResult= {
                            true,
                            1,
                            1,
                            1
        };

        mainThread.execute([&]{
            virtualMachineMock.executeCall(callRequest1, 
            std::weak_ptr<vm::VirtualMachineInternetQueryHandler>(), 
            std::weak_ptr<vm::VirtualMachineBlockchainQueryHandler>(),
            // std::shared_ptr<AsyncQueryCallback<vm::CallExecutionResult>> callback  im not sure how to create the argument for this field
            ); 
        });
    }
}