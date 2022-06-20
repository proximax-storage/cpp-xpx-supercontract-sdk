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

// ContractEnvironmentMock
class ContractEnvironmentMock: public ContractEnvironment {

private:

    ContractKey m_contractKey;
    DriveKey m_driveKey;
    std::set<ExecutorKey> m_executors;
    uint64_t m_automaticExecutionsSCLimit;
    uint64_t m_automaticExecutionsSMLimit;
    ContractConfig                              m_contractConfig;

public:

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
          public VirtualMachineEventHandler,
          public ThreadManager {

private:

    VirtualMachineEventHandler& m_virtualMachineEventHandler;
    ThreadManager m_threadManager;

public:

    VirtualMachineMock( VirtualMachineEventHandler& virtualMachineEventHandler)
                        :m_virtualMachineEventHandler(virtualMachineEventHandler) {
        m_threadManager.execute( [this] {
            //not complete
            executeCall();  
        } );
            m_virtualMachineEventHandler.onSuperContractCallExecuted();
    }

    ~VirtualMachineMock() = default;

    // not complete
    void executeCall( const ContractKey&, const CallRequest& ) {}

public: 

    // region virtual machine event handler

    void
    onSuperContractCallExecuted( const ContractKey& contractKey, const CallExecutionResult& executionResult ) override {}

};
    // endregion

// VirtualMachineEventHandlerMock
class VirtualMachineEventHandlerMock: public VirtualMachineEventHandler {

public:

    ~ VirtualMachineEventHandlerMock() = default;

    void
    onSuperContractCallExecuted( const ContractKey& contractKey, const CallExecutionResult& executionResult ) override {}
};

// StorageObserverMock
class StorageObserverMock: public StorageObserver {

public:

    ~StorageObserverMock() = default;

    void getAbsolutePath( const std::string& relativePath, std::weak_ptr<AbstractAsyncQuery<std::string>> callback) const {};
};

TEST(Example, TEST_NAME) {
    auto debugThread = std::this_thread::get_id();;
    uint64_t index = 1;
    ContractEnvironmentMock contractEnv("param");
    ExecutorEnvironmentMock executorEnv("param");
    DebugInfo debugInfo("peer", debugThread);
    DefaultBatchesManager manager(index, contractEnv, executorEnv, debugInfo); 

    VirtualMachineMock vm(VirtualMachineEventHandlerMock vmHandler);  
    
    StorageObserverMock observer;
}
}