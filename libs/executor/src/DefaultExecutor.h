/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <storage/Storage.h>
#include "supercontract/SingleThread.h"
#include "executor/Executor.h"
#include "ExecutorEnvironment.h"
#include "Contract.h"
#include <blockchain/CachedBlockchain.h>
#include <supercontract/ServiceBuilder.h>
#include <messenger/MessengerBuilder.h>
#include <virtualMachine/VirtualMachineBuilder.h>

namespace sirius::contract {

class DefaultExecutor
        : private SingleThread,
        public Executor,
        public ExecutorEnvironment {

private:

    crypto::KeyPair m_keyPair;

    ThreadManager m_threadManager;
    logging::Logger m_logger;

    boost::asio::ssl::context m_sslContext;

    std::map<ContractKey, std::unique_ptr<Contract>> m_contracts;
    std::map<DriveKey, ContractKey> m_contractsDriveKeys;

    ExecutorConfig m_config;

    std::shared_ptr<ExecutorEventHandler> m_eventHandler;
    std::shared_ptr<messenger::Messenger> m_messenger;
    std::shared_ptr<storage::Storage> m_storage;
    std::shared_ptr<blockchain::CachedBlockchain> m_blockchain;

    std::shared_ptr<vm::VirtualMachine> m_virtualMachine;

public:

    DefaultExecutor(crypto::KeyPair&& keyPair,
                    const ExecutorConfig& config,
                    std::shared_ptr<ExecutorEventHandler> eventHandler,
                    std::unique_ptr<vm::VirtualMachineBuilder>&& vmBuilder,
                    std::unique_ptr<ServiceBuilder<storage::Storage>>&& storageBuilder,
                    std::unique_ptr<ServiceBuilder<blockchain::Blockchain>>&& blockchainBuilder,
                    std::unique_ptr<messenger::MessengerBuilder>&& messengerBuilder,
                    logging::Logger&& logger);

    ~DefaultExecutor() override;

    void addContract(const ContractKey& key, AddContractRequest&& request) override;

    void addManualCall(const ContractKey& key, ManualCallRequest&& request) override;

    void addBlockInfo(uint64_t blockHeight, blockchain::Block&& block) override;

    void addBlock(const ContractKey& contractKey, uint64_t height) override;

    void removeContract(const ContractKey& key, RemoveRequest&& request) override;

    void setExecutors(const ContractKey& key, std::map<ExecutorKey, ExecutorInfo>&& executors) override;

public:

    // region message subscrbier

    void onMessageReceived(const messenger::InputMessage& message) override;

    std::set<std::string> subscriptions() override;

    // endregion

public:

    // region global environment

    ThreadManager& threadManager() override;

    logging::Logger& logger() override;

    // endregion

public:

    // region executor environment

    const crypto::KeyPair& keyPair() const override;

    std::weak_ptr<messenger::Messenger> messenger() override;

    std::weak_ptr<storage::StorageModifier> storageModifier() override;

    std::weak_ptr<blockchain::Blockchain> blockchain() override;

    ExecutorEventHandler& executorEventHandler() override;

    std::weak_ptr<vm::VirtualMachine> virtualMachine() override;

    ExecutorConfig& executorConfig() override;

    boost::asio::ssl::context& sslContext() override;

    // endregion

public:

    // region blockchain event handler

    void onEndBatchExecutionPublished(PublishedEndBatchExecutionTransactionInfo&& info) override;

    void onEndBatchExecutionSingleTransactionPublished(
            PublishedEndBatchExecutionSingleTransactionInfo&& info) override;

    void onEndBatchExecutionFailed(FailedEndBatchExecutionTransactionInfo&& info) override;

    void onStorageSynchronizedPublished(PublishedSynchronizeSingleTransactionInfo&& info) override;

	void setAutomaticExecutionsEnabledSince(const ContractKey& contractKey, uint64_t blockHeight) override;

	// endregion

private:

    void terminate();

    void onEndBatchExecutionOpinionReceived(const SuccessfulEndBatchExecutionOpinion& opinion);

    void onEndBatchExecutionOpinionReceived(const UnsuccessfulEndBatchExecutionOpinion& opinion);
};

}