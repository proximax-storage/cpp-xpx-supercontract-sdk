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
#include <storage/RPCStorage.h>

#include "executor/Executor.h"
#include "crypto/KeyPair.h"
#include "supercontract/Identifiers.h"

#include "utils/Serializer.h"
#include <magic_enum.hpp>

namespace sirius::contract {

DefaultExecutor::DefaultExecutor(crypto::KeyPair&& keyPair,
                                 const ExecutorConfig& config,
                                 std::unique_ptr<ExecutorEventHandler>&& eventHandler,
                                 const std::string& dbgPeerName)
								: m_keyPair(std::move(keyPair))
								, m_logger(config.loggerConfig(), dbgPeerName)
								, m_sslContext(boost::asio::ssl::context::tlsv12_client)
								, m_config(config)
								, m_eventHandler(std::move(eventHandler)) {
		setThreadId(m_threadManager.threadId());
		m_threadManager.execute([this] {
			m_virtualMachine =
					vm::RPCVirtualMachineBuilder().build(m_storage, *this, m_config.rpcVirtualMachineAddress());
			m_messenger = std::make_shared<messenger::RPCMessenger>(*this, m_config.rpcMessengerAddress(), *this);
		  	m_storage = std::make_shared<storage::RPCStorage>(*this, m_config.rpcStorageAddress());
		  	m_sslContext.set_default_verify_paths();
		  	m_sslContext.set_verify_mode(boost::asio::ssl::verify_peer);
		});
	}

DefaultExecutor::~DefaultExecutor() {
    m_threadManager.execute([this] {
        terminate();
    });

    m_threadManager.stop();
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

        m_contracts[key] = std::make_unique<DefaultContract>(key, std::move(request), *this);
    });
}

void DefaultExecutor::addManualCall(const ContractKey& key, CallRequestParameters&& request) {
    m_threadManager.execute([=, this, request = std::move(request)] {

        auto contractIt = m_contracts.find(key);

        if (contractIt == m_contracts.end()) {
            m_logger.critical("Added call to non-existing contract {}", key);
            return;
        }

        contractIt->second->addManualCall(request);
    });
}

void DefaultExecutor::addBlockInfo(const ContractKey& key, Block&& block) {
    m_threadManager.execute([=, this, request = std::move(block)] {

        auto contractIt = m_contracts.find(key);

        if (contractIt == m_contracts.end()) {
            m_logger.critical("Added block info to non-existing contract {}", key);
            return;
        }

        contractIt->second->addBlockInfo(block);
    });
}

void DefaultExecutor::removeContract(const ContractKey& key, RemoveRequest&& request) {
    m_threadManager.execute([=, this, request = std::move(request)] {

        auto contractIt = m_contracts.find(key);

        if (contractIt == m_contracts.end()) {
            m_logger.critical("Remove non-existing contract {}", key);
            return;
        }

        contractIt->second->removeContract(request);
    });
}

void DefaultExecutor::setExecutors(const ContractKey& key, std::map<ExecutorKey, ExecutorInfo>&& executors) {
    m_threadManager.execute([=, this, executors = std::move(executors)]() mutable {

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

std::weak_ptr<storage::StorageModifier> DefaultExecutor::storageModifier() {
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
    m_threadManager.execute([this, info = std::move(info)] {
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
    m_threadManager.execute([this, info = std::move(info)] {
        auto contractIt = m_contracts.find(info.m_contractKey);

        if (contractIt == m_contracts.end()) {

            // TODO maybe error?
            return;
        }

        contractIt->second->onEndBatchExecutionSingleTransactionPublished(info);
    });
}

void DefaultExecutor::onEndBatchExecutionFailed(FailedEndBatchExecutionTransactionInfo&& info) {
    m_threadManager.execute([this, info = std::move(info)] {
        auto contractIt = m_contracts.find(info.m_contractKey);

        contractIt->second->onEndBatchExecutionFailed(info);
    });
}

void DefaultExecutor::onStorageSynchronizedPublished(PublishedSynchronizeSingleTransactionInfo&& info) {
    m_threadManager.execute([=, this] {
        auto contractIt = m_contracts.find(info.m_contractKey);

        if (contractIt == m_contracts.end()) {

            // TODO maybe error?
            return;
        }

        contractIt->second->onStorageSynchronizedPublished(info.m_batchIndex);
    });
}

void DefaultExecutor::setAutomaticExecutionsEnabledSince(
		const ContractKey& contractKey,
		const std::optional<uint64_t>& blockHeight) {
	m_threadManager.execute([=, this] {
	  auto contractIt = m_contracts.find(contractKey);

	  if (contractIt == m_contracts.end()) {

	  	// TODO maybe error?
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
    m_storage.reset();
    m_messenger.reset();
}

void DefaultExecutor::onEndBatchExecutionOpinionReceived(const SuccessfulEndBatchExecutionOpinion& opinion) {
    if (!opinion.verify()) {
        return;
    }

    auto contractIt = m_contracts.find(opinion.m_contractKey);
    if (contractIt == m_contracts.end()) {
        contractIt->second->onEndBatchExecutionOpinionReceived(opinion);
    }
}

void DefaultExecutor::onEndBatchExecutionOpinionReceived(const UnsuccessfulEndBatchExecutionOpinion& opinion) {
    if (!opinion.verify()) {
        return;
    }

    auto contractIt = m_contracts.find(opinion.m_contractKey);
    if (contractIt == m_contracts.end()) {
        contractIt->second->onEndBatchExecutionOpinionReceived(opinion);
    }
}

}