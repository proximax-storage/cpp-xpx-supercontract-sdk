/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <storage/StorageContentManager.h>
#include "supercontract/SingleThread.h"
#include "supercontract/Executor.h"
#include "ExecutorEnvironment.h"
#include "Contract.h"

namespace sirius::contract {

class DefaultExecutor
        : private SingleThread,
        public Executor,
        public ExecutorEnvironment {

private:

    const crypto::KeyPair& m_keyPair;

    std::shared_ptr<ThreadManager> m_pThreadManager;
    logging::Logger m_logger;

    boost::asio::ssl::context m_sslContext;

    std::map<ContractKey, std::unique_ptr<Contract>> m_contracts;
    std::map<DriveKey, ContractKey> m_contractsDriveKeys;

    ExecutorConfig m_config;

    std::unique_ptr<ExecutorEventHandler> m_eventHandler;
    std::shared_ptr<messenger::Messenger> m_messenger;
    std::shared_ptr<storage::Storage> m_storage;
    std::shared_ptr<storage::StorageContentManager> m_storageContentManager;

    std::shared_ptr<vm::VirtualMachine> m_virtualMachine;

public:

    DefaultExecutor(const crypto::KeyPair& keyPair,
                    std::shared_ptr<ThreadManager> pThreadManager,
                    const ExecutorConfig& config,
                    std::unique_ptr<ExecutorEventHandler>&& eventHandler,
                    const std::string& dbgPeerName = "executor");

    ~DefaultExecutor() override;

    void addContract(const ContractKey& key, AddContractRequest&& request) override;

    void addContractCall(const ContractKey& key, CallRequest&& request) override;

    void addBlockInfo(const ContractKey& key, Block&& block) override;

    void removeContract(const ContractKey& key, RemoveRequest&& request) override;

    void setExecutors(const ContractKey& key, std::set<ExecutorKey>&& executors) override;

public:

    // region message subscrbier

    void onMessageReceived(const messenger::InputMessage& message) override;

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

    std::weak_ptr<storage::Storage> storage() override;

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

    void onStorageSynchronized(const ContractKey& contractKey, uint64_t batchIndex) override;

    // endregion

private:

    void terminate();

    void onEndBatchExecutionOpinionReceived(const EndBatchExecutionOpinion& opinion);
};

}