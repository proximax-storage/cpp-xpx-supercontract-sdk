/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "gtest/gtest.h"
#include "supercontract/batchesManagers/DefaultBatchesManager.h"
#include "contract/StorageObserver.h"
#include <thread>
#include <future>
#include <time.h>
// #include "Executor.h"

namespace sirius::contract::test {

    #define TEST_NAME Example

    // ExecutorEnvironmentMock
    class ExecutorEnvironmentMock: public ExecutorEnvironment {

    private: 

        const crypto::KeyPair& m_keyPair;
        ThreadManager m_threadManager;
        Messenger& m_messenger;
        Storage& m_storage;
        ExecutorEventHandler& m_eventHandler;
        std::shared_ptr<VirtualMachine> m_virtualMachine;
        ExecutorConfig m_config;
        std::weak_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineInternetQueryHandler>> m_virtualMachineInternetQueryKeeper;

    public:

        ExecutorEnvironmentMock(
                                crypto::KeyPair& keyPair,
                                Messenger& messenger,
                                Storage& m_storage,
                                ExecutorEventHandler& eventHandler,
                                std::shared_ptr<VirtualMachine> virtualMachine,
                                ExecutorConfig config)
                                : m_keyPair(keyPair),
                                m_messenger(messenger),
                                m_storage(m_storage),
                                m_eventHandler(eventHandler),
                                m_virtualMachine(virtualMachine),
                                m_config(config) {}

        ~ExecutorEnvironmentMock() = default;

        const crypto::KeyPair& keyPair() const override {
            return m_keyPair;
        }

        ThreadManager& threadManager() override {
            return m_threadManager;
        }

        Messenger& messenger() override {
            return m_messenger;
        }

        Storage& storage() override {
            return m_storage;
        }

        ExecutorEventHandler& executorEventHandler() override {
            return m_eventHandler;
        }

        std::weak_ptr<VirtualMachine> virtualMachine() override {
            return m_virtualMachine;
        }

        ExecutorConfig& executorConfig() override {
            return m_config;
        }

        std::weak_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineInternetQueryHandler>> internetHandlerKeeper() override {
            return m_virtualMachineInternetQueryKeeper;
        }
    };

    // Mock classes that required by ExecutorEnvironment
    class MessengerMock: public Messenger {
    public:
        ~MessengerMock() = default;
        void sendMessage(const ExecutorKey& key, const std::string& msg) {}
    };

    class StorageMock: public Storage {
    public:
        ~StorageMock() = default;
        void synchronizeStorage( const DriveKey& driveKey, const StorageHash& storageHash ) {}
        void initiateModifications( const DriveKey& driveKey, uint64_t batchIndex ) {}
        void applySandboxStorageModifications( const DriveKey& driveKey, uint64_t batchIndex, bool success ) {}
        void evaluateStorageHash( const DriveKey& driveKey, uint64_t batchIndex ) {}
        void applyStorageModifications( const DriveKey& driveKey, uint64_t batchIndex, bool success ) {}
    };

    class ExecutorEventHandlerMock: public ExecutorEventHandler {
    public:
        ~ExecutorEventHandlerMock() = default;
        void endBatchTransactionIsReady(const EndBatchExecutionTransactionInfo&) {}
        void endBatchSingleTransactionIsReady(const EndBatchExecutionSingleTransactionInfo&) {}
    };
    // End of mock classes

    // ContractEnvironmentMock
    class ContractEnvironmentMock: public ContractEnvironment {

    private:

        ContractKey m_contractKey;
        DriveKey m_driveKey;
        std::set<ExecutorKey> m_executors;
        uint64_t m_automaticExecutionsSCLimit;
        uint64_t m_automaticExecutionsSMLimit;
        ContractConfig m_contractConfig;

    public:

        ContractEnvironmentMock(ContractKey contractKey, uint64_t automaticExecutionsSCLimit, uint64_t automaticExecutionsSMLimit)
                                : m_contractKey(contractKey),
                                  m_automaticExecutionsSCLimit(automaticExecutionsSCLimit),
                                  m_automaticExecutionsSMLimit(automaticExecutionsSMLimit) {}

        ~ContractEnvironmentMock() = default;

        const ContractKey& contractKey() const override {
            return m_contractKey;
        };

        const DriveKey& driveKey() const override {
            return m_driveKey;
        };

        const std::set<ExecutorKey>& executors() const override {
            return m_executors;
        };

        uint64_t automaticExecutionsSCLimit() const override {
            return m_automaticExecutionsSCLimit;
        };

        uint64_t automaticExecutionsSMLimit() const override {
            return m_automaticExecutionsSMLimit;
        };

        const ContractConfig& contractConfig() const override {
            return m_contractConfig;
        };

        void finishTask() override {};

        void addSynchronizationTask( const SynchronizationRequest& ) override {};
    };

    // VirtualMachineMock
    class VirtualMachineMock
            : public VirtualMachine,
            public ThreadManager {

    private:

        VirtualMachineEventHandlerMock& m_virtualMachineEventHandlerMock;
        ThreadManager m_threadManager;

    public:
        // VirtualMachineMock constructor
        VirtualMachineMock( VirtualMachineEventHandlerMock& virtualMachineEventHandlerMock)  
                            :m_virtualMachineEventHandlerMock(virtualMachineEventHandlerMock) {
            srand(time(0));
            // thread call executeCall() in random time 1-10sec
            m_threadManager.startTimer(
                (rand()%10000), 
                [this] { executeCall(); }
            );
        }

        ~VirtualMachineMock() = default;

        void executeCall( const ContractKey&, const CallRequest& ) {
            // executeCall() call vmHandlerMock::onSuperContractCallExecuted()
            m_virtualMachineEventHandlerMock.onSuperContractCallExecuted();
        }
    };

    // VirtualMachineEventHandlerMock
    class VirtualMachineEventHandlerMock: public VirtualMachineEventHandler {

    private:
        DefaultBatchesManager& m_defaultBatchesManager;

    public:
        VirtualMachineEventHandlerMock( DefaultBatchesManager& defaultBatchesManager)
                                        :m_defaultBatchesManager(defaultBatchesManager){}

        ~ VirtualMachineEventHandlerMock() = default;

        void
        onSuperContractCallExecuted( const ContractKey& contractKey, const CallExecutionResult& executionResult ) override {
            // create new thread to call corresponding method of the batches manager 
            ThreadManager worker;
            worker.execute([this] {m_defaultBatchesManager.hasNextBatch();});
        }
    };

    // StorageObserverMock
    class StorageObserverMock: public StorageObserver {

    public:

        ~StorageObserverMock() = default;

        void getAbsolutePath( const std::string& relativePath, std::weak_ptr<AbstractAsyncQuery<std::string>> callback) const {

        };
    };

    TEST(Example, TEST_NAME) {
        // ContractEnvironment field
        ContractKey contractKey;
        uint64_t automaticExecutionsSCLimit = 0;
        uint64_t automaticExecutionsSMLimit = 0;

        // ExecutorEnvironment field
        crypto::PrivateKey privateKey;
        crypto::KeyPair keyPair(std::move(privateKey)); 
        
        // crypto::PrivateKey rvalue_ref = (crypto::PrivateKey&&) privateKey;
        // crypto::KeyPair keyPair(rvalue_ref); 
        // crypto::PrivateKey rvalue_ref( (crypto::PrivateKey&&) privateKey );
        // crypto::KeyPair keyPair(rvalue_ref); 
        // crypto::PrivateKey rvalue_ref = std::move(privateKey);
        // crypto::KeyPair keyPair(rvalue_ref); 

        MessengerMock messengerMock;
        StorageMock storageMock;
        ExecutorEventHandlerMock executorEventHandlerMock;
        ExecutorConfig executorConfig;

        // DefaultBatchesManager field
        ThreadManager debugThread;
        uint64_t index = 1;
        ContractEnvironmentMock contractEnv(contractKey, automaticExecutionsSCLimit, automaticExecutionsSMLimit);
        ExecutorEnvironmentMock executorEnv(keyPair, messengerMock, storageMock, executorEventHandlerMock, executorConfig);
        DebugInfo debugInfo("peer", debugThread.threadId());

        DefaultBatchesManager batchesManager(index, contractEnv, executorEnv, debugInfo); 
        VirtualMachineEventHandlerMock vmHandler(batchesManager);
        VirtualMachineMock virtualMachine(vmHandler);  
        
        StorageObserverMock observer;
    }
}