/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <boost/beast/ssl.hpp>

#include "Contract.h"
#include "supercontract/ThreadManager.h"
#include "ExecutorEnvironment.h"
#include "Messages.h"
#include "virtualMachine/RPCVirtualMachineBuilder.h"
#include "DebugInfo.h"

#include "supercontract/Executor.h"
#include "crypto/KeyPair.h"
#include "supercontract/Identifiers.h"

#include "utils/Serializer.h"
#include "utils/Random.h"
#include "log.h"

namespace sirius::contract {

class DefaultExecutor
        : private SingleThread,
          public Executor,
          public ExecutorEnvironment {

private:

    const crypto::KeyPair& m_keyPair;

    std::shared_ptr<ThreadManager> m_pThreadManager;
    logging::Logger logger;

    boost::asio::ssl::context m_sslContext;

    std::map<ContractKey, std::unique_ptr<Contract>> m_contracts;
    std::map<DriveKey, ContractKey> m_contractsDriveKeys;

    ExecutorConfig m_config;

    std::unique_ptr<ExecutorEventHandler> m_eventHandler;
    Messenger& m_messenger;
    Storage& m_storage;

    std::shared_ptr<vm::VirtualMachine> m_virtualMachine;

public:

    DefaultExecutor(const crypto::KeyPair& keyPair,
                    std::shared_ptr<ThreadManager> pThreadManager,
                    const ExecutorConfig& config,
                    std::unique_ptr<ExecutorEventHandler> eventHandler,
                    Messenger& messenger,
                    Storage& storage,
                    const StorageObserver& storageObserver,
                    const std::string& dbgPeerName = "executor")
            : m_keyPair(keyPair)
            , m_pThreadManager(std::move(pThreadManager))
            , m_sslContext(boost::asio::ssl::context::tlsv12_client)
            , m_config(config)
            , m_eventHandler(std::move(eventHandler))
            , m_messenger(messenger)
            , m_storage(storage)
            , m_virtualMachine(
                    vm::RPCVirtualMachineBuilder().build(storageObserver, *this, m_config.rpcVirtualMachineAddress())) {
        m_sslContext.set_default_verify_paths();
        m_sslContext.set_verify_mode(boost::asio::ssl::verify_peer);
    }

    ~DefaultExecutor() override {
        m_pThreadManager->execute([this] {
            for (auto&[_, contract] : m_contracts) {
                contract->terminate();
            }
        });

        m_pThreadManager->stop();
    }

    void addContract(const ContractKey& key, AddContractRequest&& request) override {
        m_pThreadManager->execute([=, this, request = std::move(request)]() mutable {
            if (m_contracts.contains(key)) {
                return;
            }

            auto it = request.m_executors.find(m_keyPair.publicKey());

            if (it == request.m_executors.end()) {
                _LOG_ERR("The Executor Is Not In List")
                return;
            }

            request.m_executors.erase(it);

            m_contracts[key] = createDefaultContract(key, std::move(request), *this, m_config);
        });
    }

    void addContractCall(const ContractKey& key, CallRequest&& request) override {
        m_pThreadManager->execute([=, this, request = std::move(request)] {

            auto contractIt = m_contracts.find(key);

            if (contractIt == m_contracts.end()) {
                _LOG_ERR("Add Call to Non-Existing Contract " << key);
                return;
            }

            contractIt->second->addContractCall(request);
        });
    }

    void addBlockInfo(const ContractKey& key, Block&& block) override {
        m_pThreadManager->execute([=, this, request = std::move(block)] {

            auto contractIt = m_contracts.find(key);

            if (contractIt == m_contracts.end()) {
                _LOG_ERR("Add Block to Non-Existing Contract " << key);
                return;
            }

            contractIt->second->addBlockInfo(block);
        });
    }

    void removeContract(const ContractKey& key, RemoveRequest&& request) override {
        m_pThreadManager->execute([=, this, request = std::move(request)] {

            auto contractIt = m_contracts.find(key);

            if (contractIt == m_contracts.end()) {
                _LOG_ERR("Remove Non-Existing Contract " << key);
                return;
            }

            contractIt->second->removeContract(request);
        });
    }

    void setExecutors(const ContractKey& key, std::set<ExecutorKey>&& executors) override {
        m_pThreadManager->execute([=, this, executors = std::move(executors)]() mutable {

            auto contractIt = m_contracts.find(key);

            auto executorIt = executors.find(m_keyPair.publicKey());

            if (executorIt == executors.end()) {
                _LOG_ERR("The Executor Is Not In List")
                return;
            }

            executors.erase(executorIt);

            contractIt->second->setExecutors(std::move(executors));
        });
    }

public:

    // region storage event handler

    void onInitiatedModifications(const DriveKey& driveKey, uint64_t batchIndex) override {
        m_pThreadManager->execute([=, this] {
            auto driveIt = m_contractsDriveKeys.find(driveKey);

            if (driveIt == m_contractsDriveKeys.end()) {
                return;
            }

            auto contractIt = m_contracts.find(driveIt->second);

            if (contractIt == m_contracts.end()) {

                // TODO maybe error?
                return;
            }

            contractIt->second->onInitiatedModifications(batchIndex);
        });
    }

    void onAppliedSandboxStorageModifications(const DriveKey& driveKey, uint64_t batchIndex, bool success,
                                              int64_t sandboxSizeDelta, int64_t stateSizeDelta) override {
        m_pThreadManager->execute([=, this] {
            auto driveIt = m_contractsDriveKeys.find(driveKey);

            if (driveIt == m_contractsDriveKeys.end()) {
                return;
            }

            auto contractIt = m_contracts.find(driveIt->second);

            if (contractIt == m_contracts.end()) {

                // TODO maybe error?
                return;
            }

            contractIt->second->onAppliedSandboxStorageModifications(batchIndex, success, sandboxSizeDelta,
                                                                     stateSizeDelta);
        });
    }

    void
    onStorageHashEvaluated(const DriveKey& driveKey, uint64_t batchIndex, const StorageHash& storageHash,
                           uint64_t usedDriveSize,
                           uint64_t metaFilesSize, uint64_t fileStructureSize) override {
        m_pThreadManager->execute([=, this] {
            auto driveIt = m_contractsDriveKeys.find(driveKey);

            if (driveIt == m_contractsDriveKeys.end()) {
                return;
            }

            auto contractIt = m_contracts.find(driveIt->second);

            if (contractIt == m_contracts.end()) {

                // TODO maybe error?
                return;
            }

            contractIt->second->onStorageHashEvaluated(batchIndex, storageHash, usedDriveSize, metaFilesSize,
                                                       fileStructureSize);
        });
    }

    // endregion

public:

    // region message event handler

    void onMessageReceived(const std::string& tag, const std::string& msg) override {
        m_pThreadManager->execute([=, this] {
            try {
                if (tag == "end_batch") {
                    auto info = utils::deserialize<EndBatchExecutionOpinion>(msg);
                    onEndBatchExecutionOpinionReceived(info);
                    return true;
                }
            } catch (...) {
                logger().warn("onMessageReceived: invalid message format: query={}", tag);
            }
            return false;
        });
    }

    // endregion

public:

    // region global environment

    ThreadManager& threadManager() override {
        return *m_pThreadManager;
    }

    logging::Logger& logger() override {
        return <#initializer#>;
    }

    // endregion

public:

    // region executor environment

    const crypto::KeyPair& keyPair() const override {
        return m_keyPair;
    }

    Messenger& messenger() override {
        return m_messenger;
    }

    Storage& storage() override {
        return m_storage;
    }

    ExecutorEventHandler& executorEventHandler() override {
        return *m_eventHandler;
    }

    std::weak_ptr<vm::VirtualMachine> virtualMachine() override {
        return m_virtualMachine;
    }

    ExecutorConfig& executorConfig() override {
        return m_config;
    }

    boost::asio::ssl::context& sslContext() override {
        return m_sslContext;
    }

    // endregion

public:

    // region blockchain event handler

    void onEndBatchExecutionPublished(PublishedEndBatchExecutionTransactionInfo&& info) override {
        m_pThreadManager->execute([this, info = std::move(info)] {
            auto contractIt = m_contracts.find(info.m_contractKey);

            if (contractIt == m_contracts.end()) {

                // TODO maybe error?
                return;
            }

            contractIt->second->onEndBatchExecutionPublished(info);
        });
    }

    void onEndBatchExecutionSingleTransactionPublished(
            PublishedEndBatchExecutionSingleTransactionInfo&& info) override {
        m_pThreadManager->execute([this, info = std::move(info)] {
            auto contractIt = m_contracts.find(info.m_contractKey);

            if (contractIt == m_contracts.end()) {

                // TODO maybe error?
                return;
            }

            contractIt->second->onEndBatchExecutionSingleTransactionPublished(info);
        });
    }

    void onEndBatchExecutionFailed(FailedEndBatchExecutionTransactionInfo&& info) override {
        m_pThreadManager->execute([this, info = std::move(info)] {
            auto contractIt = m_contracts.find(info.m_contractKey);

            contractIt->second->onEndBatchExecutionFailed(info);
        });
    }

    void onStorageSynchronized(const ContractKey& contractKey, uint64_t batchIndex) override {
        m_pThreadManager->execute([=, this] {
            auto contractIt = m_contracts.find(contractKey);

            if (contractIt == m_contracts.end()) {

                // TODO maybe error?
                return;
            }

            contractIt->second->onStorageSynchronized(batchIndex);
        });
    }

    // endregion

private:

    void terminate() {
        
        ASSERT
        
        m_virtualMachine.reset();
        for (auto&[_, contract]: m_contracts) {
            contract->terminate();
        }
    }

    void onEndBatchExecutionOpinionReceived(const EndBatchExecutionOpinion& opinion) {
        if (!opinion.hasValidForm() || !opinion.verify()) {
            return;
        }

        auto contractIt = m_contracts.find(opinion.m_contractKey);
        if (contractIt == m_contracts.end()) {
            contractIt->second->onEndBatchExecutionOpinionReceived(opinion);
        }
    }
};

}