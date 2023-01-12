/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "ExecutorEnvironment.h"
#include "VirtualMachineMock.h"
#include "storage/Storage.h"
#include "ExecutorEventHandlerMock.h"

namespace sirius::contract::test {

class ExecutorEnvironmentMock : public ExecutorEnvironment {
private:
    crypto::KeyPair m_keyPair;
    ExecutorConfig m_executorConfig;
    ExecutorEventHandlerMock m_executorEventHandlerMock;
    boost::asio::ssl::context m_sslContext{boost::asio::ssl::context::tlsv12_client};
    ThreadManager& m_threadManager;
    logging::Logger m_logger;
    std::weak_ptr<messenger::Messenger> m_messenger;

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
                            std::weak_ptr<storage::Storage> storageMock,
                            std::weak_ptr<messenger::Messenger> messengerMock);

    const crypto::KeyPair& keyPair() const override;

    std::weak_ptr<messenger::Messenger> messenger() override;

    std::weak_ptr<storage::StorageModifier> storageModifier() override;

    std::weak_ptr<blockchain::Blockchain> blockchain() override;

    ExecutorEventHandler& executorEventHandler() override;

    std::weak_ptr<vm::VirtualMachine> virtualMachine() override;

    ExecutorConfig& executorConfig() override;

    boost::asio::ssl::context& sslContext() override;

    ThreadManager& threadManager() override;

    logging::Logger& logger() override;
};
} // namespace sirius::contract::test