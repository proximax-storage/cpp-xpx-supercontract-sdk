/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "../../src/supercontract/ExecutorEnvironment.h"
#include "VirtualMachineMock.h"
#include "storage/Storage.h"

namespace sirius::contract::test {

class ExecutorEventHandlerMock : public ExecutorEventHandler {
public:
    void endBatchTransactionIsReady(const EndBatchExecutionTransactionInfo&) override {}

    void endBatchSingleTransactionIsReady(const EndBatchExecutionSingleTransactionInfo&) override {}
};

class ExecutorEnvironmentMock : public ExecutorEnvironment {
private:
    crypto::KeyPair m_keyPair;
    ExecutorConfig m_executorConfig;
    ExecutorEventHandlerMock m_executorEventHandlerMock;
    boost::asio::ssl::context m_sslContext{boost::asio::ssl::context::tlsv12_client};
    ThreadManager& m_threadManager;
    logging::Logger m_logger;

public:
    std::weak_ptr<storage::Storage> m_storage;
    std::weak_ptr<vm::VirtualMachine> m_virtualMachineMock;
    ExecutorEnvironmentMock(crypto::KeyPair&& keyPair,
                            std::weak_ptr<vm::VirtualMachine> virtualMachineMock,
                            const ExecutorConfig& executorConfig,
                            ThreadManager& threadManager);

    ExecutorEnvironmentMock(crypto::KeyPair&& keyPair,
                            std::weak_ptr<vm::VirtualMachine> virtualMachineMock,
                            const ExecutorConfig& executorConfig,
                            ThreadManager& threadManager,
                            std::weak_ptr<storage::Storage> storageMock);

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
} // namespace sirius::contract::test