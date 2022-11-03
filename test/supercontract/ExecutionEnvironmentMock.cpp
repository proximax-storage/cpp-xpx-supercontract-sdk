/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ExecutionEnvironmentMock.h"

namespace sirius::contract::test {

ExecutorEnvironmentMock::ExecutorEnvironmentMock(crypto::KeyPair&& keyPair,
                                                 std::weak_ptr<VirtualMachineMock> virtualMachineMock,
                                                 const ExecutorConfig& executorConfig,
                                                 ThreadManager& threadManager)
        : m_keyPair(std::move(keyPair)), m_virtualMachineMock(std::move(virtualMachineMock)), m_executorConfig(executorConfig)
        , m_threadManager(threadManager), m_logger(executorConfig.loggerConfig(), "executor") {}

const crypto::KeyPair& ExecutorEnvironmentMock::keyPair() const { return m_keyPair; }

Messenger& ExecutorEnvironmentMock::messenger() { return m_messengerMock; }

std::weak_ptr<storage::Storage> ExecutorEnvironmentMock::storage() { return m_storage; }

ExecutorEventHandler& ExecutorEnvironmentMock::executorEventHandler() { return m_executorEventHandlerMock; }

std::weak_ptr<vm::VirtualMachine> ExecutorEnvironmentMock::virtualMachine() { return m_virtualMachineMock; }

ExecutorConfig& ExecutorEnvironmentMock::executorConfig() { return m_executorConfig; }

boost::asio::ssl::context& ExecutorEnvironmentMock::sslContext() { return m_sslContext; }

ThreadManager& ExecutorEnvironmentMock::threadManager() { return m_threadManager; }

logging::Logger& ExecutorEnvironmentMock::logger() { return m_logger; }
}