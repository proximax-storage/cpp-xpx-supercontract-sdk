/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "DefaultExecutor.h"

#include <boost/beast/ssl.hpp>

#include "supercontract/ThreadManager.h"
#include "ExecutorEnvironment.h"
#include "Messages.h"
#include <virtualMachine/RPCVirtualMachineBuilder.h>
#include "DefaultContract.h"
#include <messenger/RPCMessenger.h>

#include "supercontract/Executor.h"
#include "crypto/KeyPair.h"
#include "supercontract/Identifiers.h"

#include "utils/Serializer.h"
#include <magic_enum.hpp>

namespace sirius::contract {

DefaultExecutor::DefaultExecutor(const crypto::KeyPair& keyPair,
                                 std::shared_ptr<ThreadManager> pThreadManager,
                                 const ExecutorConfig& config,
                                 std::unique_ptr<ExecutorEventHandler>&& eventHandler,
                                 const std::string& dbgPeerName)
        : m_keyPair(keyPair)
        , m_pThreadManager(std::move(pThreadManager))
        , m_logger(config.loggerConfig(), dbgPeerName)
        , m_sslContext(boost::asio::ssl::context::tlsv12_client)
        , m_config(config)
        , m_eventHandler(std::move(eventHandler))
        // TODO Init pointers
        , m_storage(nullptr)
        , m_storageContentManager(nullptr)
        , m_messenger(std::make_shared<messenger::RPCMessenger>(*this, m_config.rpcMessengerAddress(), *this))
        , m_virtualMachine(
                vm::RPCVirtualMachineBuilder().build(m_storageContentManager, *this, m_config.rpcVirtualMachineAddress())) {
    m_sslContext.set_default_verify_paths();
    m_sslContext.set_verify_mode(boost::asio::ssl::verify_peer);
}

DefaultExecutor::~DefaultExecutor() {
    m_pThreadManager->execute([this] {
        terminate();
    });

    m_pThreadManager->stop();
}

void DefaultExecutor::addContract(const ContractKey& key, AddContractRequest&& request) {
    m_pThreadManager->execute([=, this, request = std::move(request)]() mutable {
        if (m_contracts.contains(key)) {
            return;
        }

        auto it = request.m_executors.find(m_keyPair.publicKey());

        if (it == request.m_executors.end()) {
            m_logger.critical("The executor is not in the list of {} contract", key);
            return;
        }

        request.m_executors.erase(it);

        m_contracts[key] = std::make_unique<DefaultContract>(key, std::move(request), *this);
    });
}

void DefaultExecutor::addContractCall(const ContractKey& key, CallRequest&& request) {
    m_pThreadManager->execute([=, this, request = std::move(request)] {

        auto contractIt = m_contracts.find(key);

        if (contractIt == m_contracts.end()) {
            m_logger.critical("Added call to non-existing contract {}", key);
            return;
        }

        contractIt->second->addContractCall(request);
    });
}

void DefaultExecutor::addBlockInfo(const ContractKey& key, Block&& block) {
    m_pThreadManager->execute([=, this, request = std::move(block)] {

        auto contractIt = m_contracts.find(key);

        if (contractIt == m_contracts.end()) {
            m_logger.critical("Added block info to non-existing contract {}", key);
            return;
        }

        contractIt->second->addBlockInfo(block);
    });
}

void DefaultExecutor::removeContract(const ContractKey& key, RemoveRequest&& request) {
    m_pThreadManager->execute([=, this, request = std::move(request)] {

        auto contractIt = m_contracts.find(key);

        if (contractIt == m_contracts.end()) {
            m_logger.critical("Remove non-existing contract {}", key);
            return;
        }

        contractIt->second->removeContract(request);
    });
}

void DefaultExecutor::setExecutors(const ContractKey& key, std::set<ExecutorKey>&& executors) {
    m_pThreadManager->execute([=, this, executors = std::move(executors)]() mutable {

        auto contractIt = m_contracts.find(key);

        auto executorIt = executors.find(m_keyPair.publicKey());

        if (executorIt == executors.end()) {
            m_logger.critical("Executor is not in the updated list of contract executors", key);
            return;
        }

        executors.erase(executorIt);

        contractIt->second->setExecutors(std::move(executors));
    });
}

// region message subscriber

void DefaultExecutor::onMessageReceived(const messenger::InputMessage& inputMessage) {
    m_pThreadManager->execute([=, this] {
        try {
            auto tag = magic_enum::enum_cast<MessageTag>(inputMessage.m_tag);
            if (tag.has_value()) {
                switch (tag.value()) {
                    case MessageTag::END_BATCH : {
                        auto info = utils::deserialize<EndBatchExecutionOpinion>(inputMessage.m_content);
                        onEndBatchExecutionOpinionReceived(info);
                        break;
                    }
                }
            }
            else {
                logger().warn("onMessageReceived: unknown tag", inputMessage.m_tag);
            }
        } catch (...) {
            logger().warn("onMessageReceived: invalid message format: query={}", inputMessage.m_content);
        }
    });
}

std::set<std::string> DefaultExecutor::subscriptions() {

    constexpr auto values = magic_enum::enum_names<MessageTag>();

    std::set<std::string> tags;

    for (const auto& v: values) {
        tags.emplace(v.begin(), v.end());
    }

    return tags;
}

// endregion

// region global environment

ThreadManager& DefaultExecutor::threadManager() {
    return *m_pThreadManager;
}

logging::Logger& DefaultExecutor::logger() {
    return m_logger;
}

// endregion

// region executor environment

const crypto::KeyPair& DefaultExecutor::keyPair() const {
    return m_keyPair;
}

std::weak_ptr<messenger::Messenger> DefaultExecutor::messenger() {
    return m_messenger;
}

std::weak_ptr<storage::Storage> DefaultExecutor::storage() {
    return m_storage;
}

ExecutorEventHandler& DefaultExecutor::executorEventHandler() {
    return *m_eventHandler;
}

std::weak_ptr<vm::VirtualMachine> DefaultExecutor::virtualMachine() {
    return m_virtualMachine;
}

ExecutorConfig& DefaultExecutor::executorConfig() {
    return m_config;
}

boost::asio::ssl::context& DefaultExecutor::sslContext() {
    return m_sslContext;
}

// endregion

// region blockchain event handler

void DefaultExecutor::onEndBatchExecutionPublished(PublishedEndBatchExecutionTransactionInfo&& info) {
    m_pThreadManager->execute([this, info = std::move(info)] {
        auto contractIt = m_contracts.find(info.m_contractKey);

        if (contractIt == m_contracts.end()) {

            // TODO maybe error?
            return;
        }

        contractIt->second->onEndBatchExecutionPublished(info);
    });
}

void DefaultExecutor::onEndBatchExecutionSingleTransactionPublished(
        PublishedEndBatchExecutionSingleTransactionInfo&& info) {
    m_pThreadManager->execute([this, info = std::move(info)] {
        auto contractIt = m_contracts.find(info.m_contractKey);

        if (contractIt == m_contracts.end()) {

            // TODO maybe error?
            return;
        }

        contractIt->second->onEndBatchExecutionSingleTransactionPublished(info);
    });
}

void DefaultExecutor::onEndBatchExecutionFailed(FailedEndBatchExecutionTransactionInfo&& info) {
    m_pThreadManager->execute([this, info = std::move(info)] {
        auto contractIt = m_contracts.find(info.m_contractKey);

        contractIt->second->onEndBatchExecutionFailed(info);
    });
}

void DefaultExecutor::onStorageSynchronized(const ContractKey& contractKey, uint64_t batchIndex) {
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

void DefaultExecutor::terminate() {

    ASSERT(isSingleThread(), m_logger)
    m_messenger = std::make_shared<messenger::RPCMessenger>(*this, "", *this);
    for (auto&[_, contract]: m_contracts) {
        contract->terminate();
    }

    m_virtualMachine.reset();
}

void DefaultExecutor::onEndBatchExecutionOpinionReceived(const EndBatchExecutionOpinion& opinion) {
    if (!opinion.hasValidForm() || !opinion.verify()) {
        return;
    }

    auto contractIt = m_contracts.find(opinion.m_contractKey);
    if (contractIt == m_contracts.end()) {
        contractIt->second->onEndBatchExecutionOpinionReceived(opinion);
    }
}

}