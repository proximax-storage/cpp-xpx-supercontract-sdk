/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ExecutorEnvironmentMock.h"

#include <utility>

namespace sirius::contract::test {
ExecutorEnvironmentMock::ExecutorEnvironmentMock(crypto::KeyPair&& keyPair,
                                                 std::weak_ptr<vm::VirtualMachine> virtualMachineMock,
                                                 const ExecutorConfig& executorConfig,
                                                 ThreadManager& threadManager)
        : m_keyPair(std::move(keyPair)), m_executorConfig(
        executorConfig), m_threadManager(threadManager), m_logger(logging::LoggerConfig(), "executor"), m_virtualMachineMock(std::move(virtualMachineMock)) {}

ExecutorEnvironmentMock::ExecutorEnvironmentMock(crypto::KeyPair&& keyPair,
                                                 std::weak_ptr<vm::VirtualMachine> virtualMachineMock,
                                                 const ExecutorConfig& executorConfig,
                                                 ThreadManager& threadManager,
                                                 std::weak_ptr<storage::Storage> storageMock,
                                                 std::weak_ptr<messenger::Messenger> messengerMock,
                                                 std::shared_ptr<ExecutorEventHandler> handler)
        : m_keyPair(std::move(keyPair))
        , m_executorConfig(executorConfig)
        , m_executorEventHandlerMock(std::move(handler))
        , m_threadManager(threadManager)
        , m_logger(logging::LoggerConfig(), "executor")
        , m_messenger(std::move(messengerMock))
        , m_storage(std::move(storageMock))
        , m_virtualMachineMock(std::move(virtualMachineMock)) {}

const crypto::KeyPair& ExecutorEnvironmentMock::keyPair() const { return m_keyPair; }

ExecutorEventHandler& ExecutorEnvironmentMock::executorEventHandler() { return *m_executorEventHandlerMock; }

std::weak_ptr<vm::VirtualMachine> ExecutorEnvironmentMock::virtualMachine() { return m_virtualMachineMock; }

ExecutorConfig& ExecutorEnvironmentMock::executorConfig() { return m_executorConfig; }

boost::asio::ssl::context& ExecutorEnvironmentMock::sslContext() { return m_sslContext; }

ThreadManager& ExecutorEnvironmentMock::threadManager() { return m_threadManager; }

logging::Logger& ExecutorEnvironmentMock::logger() { return m_logger; }

std::weak_ptr<messenger::Messenger> ExecutorEnvironmentMock::messenger() {
    return m_messenger;
}

std::weak_ptr<storage::Storage> ExecutorEnvironmentMock::storage() {
    return m_storage;
}

std::weak_ptr<blockchain::Blockchain> ExecutorEnvironmentMock::blockchain() {
    return std::weak_ptr<blockchain::Blockchain>();
}
} // namespace sirius::contract::test