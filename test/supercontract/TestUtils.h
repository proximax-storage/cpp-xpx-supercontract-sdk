/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <storage/Storage.h>
#include "../../src/supercontract/ContractEnvironment.h"
#include "virtualMachine/VirtualMachine.h"
#include "../../src/supercontract/ExecutorEnvironment.h"

namespace sirius::contract::test {

class GlobalEnvironmentMock : public GlobalEnvironment {

private:
    ThreadManager m_threadManager;
    logging::Logger m_logger;

public:
    GlobalEnvironmentMock();

    ThreadManager& threadManager() override;

    logging::Logger& logger() override;

private:
    logging::LoggerConfig getLoggerConfig();
};

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
                            uint64_t automaticExecutionsSMLimit);

    const ContractKey& contractKey() const override;

    const DriveKey& driveKey() const override;

    const std::set<ExecutorKey>& executors() const override;

    uint64_t automaticExecutionsSCLimit() const override;

    uint64_t automaticExecutionsSMLimit() const override;

    const ContractConfig& contractConfig() const override;

    void finishTask() override {}

    void addSynchronizationTask() override {}

    void delayBatchExecution(Batch batch) override {}
};

class ExecutorEventHandlerMock : public ExecutorEventHandler {
public:
    void endBatchTransactionIsReady(const EndBatchExecutionTransactionInfo&) override {}

    void endBatchSingleTransactionIsReady(const EndBatchExecutionSingleTransactionInfo&) override {}
};

class VirtualMachineMock : public vm::VirtualMachine {
private:
    ThreadManager& m_threadManager;
    std::deque<bool> m_result;
    std::map<CallId, Timer> m_timers;

public:
    VirtualMachineMock(ThreadManager& threadManager, std::deque<bool> result);

    void executeCall(const vm::CallRequest& request,
                     std::weak_ptr<vm::VirtualMachineInternetQueryHandler> internetQueryHandler,
                     std::weak_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                     std::weak_ptr<vm::VirtualMachineStorageQueryHandler> storageQueryHandler,
                     std::shared_ptr<AsyncQueryCallback<vm::CallExecutionResult>> callback) override;
};

class ExecutorEnvironmentMock : public ExecutorEnvironment {
private:
    crypto::KeyPair m_keyPair;
    std::weak_ptr<VirtualMachineMock> m_virtualMachineMock;
    ExecutorConfig m_executorConfig;
    std::weak_ptr<storage::Storage> m_storage;
    ExecutorEventHandlerMock m_executorEventHandlerMock;
    boost::asio::ssl::context m_sslContext{boost::asio::ssl::context::tlsv12_client};
    ThreadManager& m_threadManager;
    logging::Logger m_logger;


public:
    ExecutorEnvironmentMock(crypto::KeyPair&& keyPair,
                            std::weak_ptr<VirtualMachineMock> virtualMachineMock,
                            const ExecutorConfig& executorConfig,
                            ThreadManager& threadManager);

    const crypto::KeyPair& keyPair() const override;

    std::weak_ptr<messenger::Messenger> messenger() override;

    std::weak_ptr<storage::StorageModifier> storageModifier() override;

    ExecutorEventHandler& executorEventHandler() override;

    std::weak_ptr<vm::VirtualMachine> virtualMachine() override;

    ExecutorConfig& executorConfig() override;

    boost::asio::ssl::context& sslContext() override;

    ThreadManager& threadManager() override;

    logging::Logger& logger() override;
};
}
