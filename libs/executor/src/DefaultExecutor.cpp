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
#include <messenger/MessengerBuilder.h>
#include <blockchain/Blockchain.h>
#include "executor/Executor.h"
#include "crypto/KeyPair.h"
#include "supercontract/Identifiers.h"

#include "utils/Serializer.h"
#include <magic_enum.hpp>

namespace sirius::contract {


DefaultExecutor::DefaultExecutor(crypto::KeyPair&& keyPair,
                                 const ExecutorConfig& config,
                                 std::shared_ptr<ExecutorEventHandler> eventHandler,
                                 std::unique_ptr<vm::VirtualMachineBuilder>&& vmBuilder,
                                 std::unique_ptr<ServiceBuilder<storage::Storage>>&& storageBuilder,
                                 std::unique_ptr<ServiceBuilder<blockchain::Blockchain>>&& blockchainBuilder,
                                 std::unique_ptr<messenger::MessengerBuilder>&& messengerBuilder,
                                 logging::Logger&& logger)
        : m_keyPair(std::move(keyPair))
          , m_logger(std::move(logger))
          , m_sslContext(boost::asio::ssl::context::tlsv12_client)
          , m_config(config)
          , m_eventHandler(std::move(eventHandler)) {
    setThreadId(m_threadManager.threadId());
    m_threadManager.execute([this,
                                    vmBuilder = std::move(vmBuilder),
                                    storageBuilder = std::move(storageBuilder),
                                    blockchainBuilder = std::move(blockchainBuilder),
                                    messengerBuilder = std::move(messengerBuilder)] {
        messengerBuilder->setMessageSubscriber(this);
        m_messenger = messengerBuilder->build(*this);

        m_storage = storageBuilder->build(*this);

        m_blockchain = std::make_shared<blockchain::CachedBlockchain>(*this, blockchainBuilder->build(*this));

        std::map<vm::CallRequest::CallLevel, uint64_t> maxExecutableSizes;
        maxExecutableSizes[vm::CallRequest::CallLevel::AUTORUN] = m_config.maxAutorunExecutableSize();
        maxExecutableSizes[vm::CallRequest::CallLevel::AUTOMATIC] = m_config.maxAutomaticExecutableSize();
        maxExecutableSizes[vm::CallRequest::CallLevel::MANUAL] = m_config.maxManualExecutableSize();
        vmBuilder->setMaxExecutableSizes(maxExecutableSizes);
        vmBuilder->setStorage(m_storage);
        m_virtualMachine = vmBuilder->build(*this);

        m_sslContext.set_default_verify_paths();
        m_sslContext.set_verify_mode(boost::asio::ssl::verify_peer);

        m_logger.info("Executor is started. Key: {}", m_keyPair.publicKey());
    });
}

DefaultExecutor::~DefaultExecutor() {

    m_logger.info("Started executor destruction");

    m_threadManager.execute([this] {
        terminate();
    });

    m_threadManager.stop();

    m_logger.info("Executor is destructed");
}

void DefaultExecutor::addContract(const ContractKey& key, AddContractRequest&& request) {
    m_threadManager.execute([=, this, request = std::move(request)]() mutable {
        if (m_contracts.contains(key)) {
            return;
        }

        auto it = request.m_executors.find(m_keyPair.publicKey());

        if (it == request.m_executors.end()) {
            m_logger.critical("The executor is not in the list of {} contract", key);
            return;
        }

        request.m_executors.erase(it);

        m_logger.info("New contract is added: Key: {}", key);
        m_contracts[key] = std::make_unique<DefaultContract>(key, std::move(request), *this);
    });
}

void DefaultExecutor::addManualCall(const ContractKey& key, ManualCallRequest&& request) {
    m_threadManager.execute([=, this, request = std::move(request)] {

        auto contractIt = m_contracts.find(key);

        if (contractIt == m_contracts.end()) {
            m_logger.critical("Added call to non-existing contract {}", key);
            return;
        }

        contractIt->second->addManualCall(request);
    });
}

void DefaultExecutor::addBlockInfo(uint64_t blockHeight, blockchain::Block&& block) {
    m_threadManager.execute([=, this, block = std::move(block)] {

        m_logger.debug("New block is added. Height: {}", blockHeight);

        m_blockchain->addBlock(blockHeight, block);
    });
}

void DefaultExecutor::addBlock(const ContractKey& contractKey, uint64_t height) {
    m_threadManager.execute([=, this] {

        auto contractIt = m_contracts.find(contractKey);

        if (contractIt == m_contracts.end()) {
            m_logger.critical("Added block info to non-existing contract {}", contractKey);
            return;
        }

        contractIt->second->onBlockPublished(height);
    });
}

void DefaultExecutor::removeContract(const ContractKey& key, RemoveRequest&& request) {
    m_threadManager.execute([=, this, request = std::move(request)] {

        auto contractIt = m_contracts.find(key);

        if (contractIt == m_contracts.end()) {
            return;
        }

        auto[query, callback] = createAsyncQuery<void>([this, key](auto&&) {
            m_contracts.erase(key);
        }, [] {}, *this, false, false);

        m_logger.info("Contract is removed: Key: {}", key);
        contractIt->second->removeContract(request, std::move(callback));
    });
}

void DefaultExecutor::setExecutors(const ContractKey& key, std::map<ExecutorKey, ExecutorInfo>&& executors) {
    m_threadManager.execute([=, this, executors = std::move(executors)]() mutable {

        auto contractIt = m_contracts.find(key);

        if (contractIt == m_contracts.end()) {
            return;
        }

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
    m_threadManager.execute([=, this] {
        try {
            auto tag = magic_enum::enum_cast<MessageTag>(inputMessage.m_tag);
            if (tag.has_value()) {
                switch (tag.value()) {
                    case MessageTag::SUCCESSFUL_END_BATCH: {
                        auto info = utils::deserialize<SuccessfulEndBatchExecutionOpinion>(inputMessage.m_content);
                        onEndBatchExecutionOpinionReceived(info);
                        break;
                    }
                    case MessageTag::UNSUCCESSFUL_END_BATCH: {
                        auto info = utils::deserialize<UnsuccessfulEndBatchExecutionOpinion>(inputMessage.m_content);
                        onEndBatchExecutionOpinionReceived(info);
                        break;
                    }
                }
            } else {
                m_logger.warn("Received unknown tag message: {}", inputMessage.m_tag);
            }
        } catch (...) {
            logger().warn("Received malformed message. Tag: {}", inputMessage.m_tag);
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
    return m_threadManager;
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

std::weak_ptr<blockchain::Blockchain> DefaultExecutor::blockchain() {
    return m_blockchain;
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

void DefaultExecutor::onEndBatchExecutionPublished(blockchain::PublishedEndBatchExecutionTransactionInfo&& info) {
    m_threadManager.execute([this, info = std::move(info)] {
        auto contractIt = m_contracts.find(info.m_contractKey);

        if (contractIt == m_contracts.end()) {
            return;
        }

        contractIt->second->onEndBatchExecutionPublished(info);
    });
}

void DefaultExecutor::onEndBatchExecutionSingleTransactionPublished(
        blockchain::PublishedEndBatchExecutionSingleTransactionInfo&& info) {
    m_threadManager.execute([this, info = std::move(info)] {
        auto contractIt = m_contracts.find(info.m_contractKey);

        if (contractIt == m_contracts.end()) {
            return;
        }

        contractIt->second->onEndBatchExecutionSingleTransactionPublished(info);
    });
}

void DefaultExecutor::onEndBatchExecutionFailed(blockchain::FailedEndBatchExecutionTransactionInfo&& info) {
    m_threadManager.execute([this, info = std::move(info)] {
        auto contractIt = m_contracts.find(info.m_contractKey);

        if (contractIt == m_contracts.end()) {
            return;
        }

        contractIt->second->onEndBatchExecutionFailed(info);
    });
}

void DefaultExecutor::onStorageSynchronizedPublished(blockchain::PublishedSynchronizeSingleTransactionInfo&& info) {
    m_threadManager.execute([=, this] {
        auto contractIt = m_contracts.find(info.m_contractKey);

        if (contractIt == m_contracts.end()) {
            return;
        }

        contractIt->second->onStorageSynchronizedPublished(info.m_batchIndex);
    });
}

void DefaultExecutor::setAutomaticExecutionsEnabledSince(
        const ContractKey& contractKey,
        uint64_t blockHeight) {
    m_threadManager.execute([=, this] {
        auto contractIt = m_contracts.find(contractKey);

        if (contractIt == m_contracts.end()) {
            return;
        }

        contractIt->second->setAutomaticExecutionsEnabledSince(blockHeight);
    });
}

// endregion

void DefaultExecutor::terminate() {

    ASSERT(isSingleThread(), m_logger)
    for (auto&[_, contract]: m_contracts) {
        contract->terminate();
    }

    m_virtualMachine.reset();
    m_blockchain.reset();
    m_storage.reset();
    m_messenger.reset();
}

void DefaultExecutor::onEndBatchExecutionOpinionReceived(const SuccessfulEndBatchExecutionOpinion& opinion) {
    if (!opinion.verify()) {
        return;
    }

    auto contractIt = m_contracts.find(opinion.m_contractKey);

    if (contractIt == m_contracts.end()) {
        return;
    }

    contractIt->second->onEndBatchExecutionOpinionReceived(opinion);
}

void DefaultExecutor::onEndBatchExecutionOpinionReceived(const UnsuccessfulEndBatchExecutionOpinion& opinion) {
    if (!opinion.verify()) {
        return;
    }

    auto contractIt = m_contracts.find(opinion.m_contractKey);

    if (contractIt == m_contracts.end()) {
        return;
    }

    contractIt->second->onEndBatchExecutionOpinionReceived(opinion);
}

}