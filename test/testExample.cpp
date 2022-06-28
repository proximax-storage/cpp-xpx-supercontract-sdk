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

        ExecutorEnvironmentMock(crypto::KeyPair& keyPair,
                                Messenger& messenger,
                                Storage& storage,
                                ExecutorEventHandler& eventHandler,
                                std::shared_ptr<VirtualMachine> virtualMachine,
                                ExecutorConfig config)
                : m_keyPair( keyPair ),
                m_messenger( messenger ),
                m_storage( storage ),
                m_eventHandler( eventHandler ),
                m_virtualMachine( virtualMachine ),
                m_config( config ) 
                {}

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
        void sendMessage( const ExecutorKey& key, const std::string& msg ) {}
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

    class VmMock: public VirtualMachine {
    public:
        ~VmMock() = default;
        void executeCall( const ContractKey&, const CallRequest& ) {}
    };
    // End of ExecutorEnvironmentMock's mock classes

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

        ContractEnvironmentMock( ContractKey contractKey, 
                                uint64_t automaticExecutionsSCLimit, 
                                uint64_t automaticExecutionsSMLimit)
                : m_contractKey( contractKey ),
                m_automaticExecutionsSCLimit( automaticExecutionsSCLimit ),
                m_automaticExecutionsSMLimit( automaticExecutionsSMLimit ) 
                {}

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
    class VirtualMachineMock: public VirtualMachine {

    private:

        DefaultBatchesManager m_batchesManager;

    public:

        VirtualMachineMock( DefaultBatchesManager batchesManager )
                :m_batchesManager(batchesManager) {}

        ~VirtualMachineMock() = default;

        void executeCall( const ContractKey&, const CallRequest& ) {
            // Create call executed result for onSuperContractCallExecuted()
            CallId callId;
            CallExecutionResult callExecutionResult;
            callExecutionResult.m_callId = callId;
            callExecutionResult.m_success = true;
            callExecutionResult.m_return = 0;
            callExecutionResult.m_scConsumed = 0;
            callExecutionResult.m_smConsumed = 0;

            m_batchesManager.onSuperContractCallExecuted(callExecutionResult);   
        }
    };

    // StorageObserverMock
    class StorageObserverMock: public StorageObserver {

    public:

        ~StorageObserverMock() = default;

        void getAbsolutePath( const std::string& relativePath, std::weak_ptr<AbstractAsyncQuery<std::string>> callback) const {

        };
    };

    void foo(std::string x){}
    void foo2(){}

    TEST(Example, TEST_NAME) {
        // ContractEnvironment field needed for batchesManager creation
        ContractKey contractKey;
        uint64_t automaticExecutionsSCLimit = 0;
        uint64_t automaticExecutionsSMLimit = 0;

        // ExecutorEnvironment field needed for batchesManager creation
        crypto::PrivateKey privateKey;
        auto keyPair = crypto::KeyPair::FromPrivate( std::move( privateKey ) );
        MessengerMock messengerMock;
        StorageMock storageMock;
        ExecutorEventHandlerMock executorEventHandlerMock;
        auto vmMock = std::make_shared<VmMock>();
        ExecutorConfig executorConfig;

        // DefaultBatchesManager field
        ThreadManager debugThread;
        uint64_t index = 1;
        ContractEnvironmentMock contractEnv( contractKey, automaticExecutionsSCLimit, automaticExecutionsSMLimit );
        ExecutorEnvironmentMock executorEnv( keyPair, messengerMock, storageMock, executorEventHandlerMock, vmMock, executorConfig );
        DebugInfo debugInfo( "peer", debugThread.threadId() );

        // Create batchesManager and virtual machine object
        DefaultBatchesManager batchesManager( index, contractEnv, executorEnv, debugInfo ); 
        VirtualMachineMock virtualMachine( batchesManager );  
        
        // Call request for batchesManager::addCall()
        CallRequest callRequest;
        CallId callId;
        std::vector<uint8_t> params = {0, 1, 2, 3};
        callRequest.m_callId = callId;
        callRequest.m_file = "";
        callRequest.m_function = "";
        callRequest.m_params = params;
        callRequest.m_scLimit = 0;
        callRequest.m_smLimit = 0;
        callRequest.m_callLevel = CallRequest::CallLevel::MANUAL;

        // Block for batchesManager::addBlockInfo()
        BlockHash blockHash;
        Block block;
        block.m_blockHash = blockHash;
        block.m_height = 0;
        Block block2;
        block.m_blockHash = blockHash;
        block.m_height = 1;
        Block block3;
        block.m_blockHash = blockHash;
        block.m_height = 2;
        Block block4;
        block.m_blockHash = blockHash;
        block.m_height = 3;

        // Call addCall() and addBlockInfo()
        batchesManager.addCall(callRequest);
        batchesManager.addCall(callRequest);
        batchesManager.addBlockInfo(block);
        batchesManager.addBlockInfo(block2);
        batchesManager.addBlockInfo(block3);
        batchesManager.addCall(callRequest);
        batchesManager.addCall(callRequest);
        batchesManager.addBlockInfo(block4);
        batchesManager.setAutomaticExecutionsEnabledSince(0);

        // ThreadManagers
        ThreadManager threadManagerForQuery;
        ThreadManager threadManager1;
        ThreadManager threadManager2;

        StorageObserverMock storageObserver;

        auto path = "/";
        auto query = std::make_shared<AbstractAsyncQuery<std::string>>(foo, foo2, threadManagerForQuery);

        threadManager1.startTimer(1000, [&] {
            storageObserver.getAbsolutePath(path, query);
        });

        // srand( time( 0 ) );
        // m_threadManager.startTimer( ( rand()%10000 ), [&] {       
        threadManager2.startTimer( 1500 , [&] {
            // Create call request for executeCall()
            CallRequest callRequest;
            CallId callId;
            std::vector<uint8_t> params = {0, 1, 2, 3};
            callRequest.m_callId = callId;
            callRequest.m_file = "";
            callRequest.m_function = "";
            callRequest.m_params = params;
            callRequest.m_scLimit = 0;
            callRequest.m_smLimit = 0;
            callRequest.m_callLevel = CallRequest::CallLevel::MANUAL;

            ContractKey contractKey;

            //call executeCall
            virtualMachine.executeCall(contractKey, callRequest);
        }); 

        std::cout << "Test start" << std::endl;
        std::cout << "next batch()" << std::endl;
        auto batch = batchesManager.nextBatch();
        std::cout << "batch index: " << batch.m_batchIndex << std::endl;
        std::cout << "has Next :" << batchesManager.hasNextBatch() << std::endl;

        std::cout << "next batch()" << std::endl;
        auto batch2 = batchesManager.nextBatch();
        std::cout << "batch index: " << batch2.m_batchIndex << std::endl;
        std::cout << "has Next :" << batchesManager.hasNextBatch() << std::endl;

    }
}