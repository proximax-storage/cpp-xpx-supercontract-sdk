/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "TestUtils.h"

namespace sirius::contract::test {
//ContractEnvironmentMock
ContractEnvironmentMock::ContractEnvironmentMock(ContractKey& contractKey,
uint64_t automaticExecutionsSCLimit,
        uint64_t automaticExecutionsSMLimit)
: m_contractKey(contractKey), m_automaticExecutionsSCLimit(automaticExecutionsSCLimit)
, m_automaticExecutionsSMLimit(automaticExecutionsSMLimit) {}

const ContractKey &ContractEnvironmentMock::contractKey() const { return m_contractKey; }
const DriveKey &ContractEnvironmentMock::driveKey() const { return m_driveKey; }
const std::set<ExecutorKey> &ContractEnvironmentMock::executors() const { return m_executors; }
uint64_t ContractEnvironmentMock::automaticExecutionsSCLimit() const { return m_automaticExecutionsSCLimit; }
uint64_t ContractEnvironmentMock::automaticExecutionsSMLimit() const { return m_automaticExecutionsSMLimit; }
const ContractConfig &ContractEnvironmentMock::contractConfig() const { return m_contractConfig; }

// VirtualMachineMock
VirtualMachineMock::VirtualMachineMock(ThreadManager& threadManager, std::deque<bool> result)
: m_threadManager(threadManager), m_result(std::move(result)) {}

void VirtualMachineMock::executeCall(const vm::CallRequest& request,
                 std::weak_ptr<vm::VirtualMachineInternetQueryHandler> internetQueryHandler,
                 std::weak_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                 std::shared_ptr<AsyncQueryCallback<vm::CallExecutionResult>> callback) {
    auto executionResult = m_result.front();
    m_result.pop_front();
    m_timers[request.m_callId] = m_threadManager.startTimer(rand()%1000, [=, this]() mutable {
    vm::CallExecutionResult result{
            executionResult,
            0,
            0,
            0,
    };
    callback->postReply(result);
    });
}

// ExecutorEnvironmentMock
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