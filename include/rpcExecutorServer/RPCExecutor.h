/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <executor/Executor.h>
#include <boost/process.hpp>
#include "executor.grpc.pb.h"
#include <executor/ExecutorEventHandler.h>
#include <common/GlobalEnvironment.h>

namespace sirius::contract::rpcExecutorServer {

class RPCExecutor: public Executor, GlobalEnvironment {

private:

    std::unique_ptr<ExecutorEventHandler> m_eventHandler;

    executor_server::ExecutorServer::AsyncService m_service;

    grpc::ServerContext m_serverContext;
    grpc::ServerAsyncReaderWriter<executor_server::ServerMessage, executor_server::ClientMessage> m_stream;

    std::unique_ptr<grpc::ServerCompletionQueue> m_completionQueue;
    std::unique_ptr<grpc::Server> m_serviceServer;
    std::thread m_completionQueueThread;

    std::thread m_readThread;

    boost::process::child m_childReplicatorProcess;

    std::mutex m_sendMutex;

public:

    RPCExecutor(std::shared_ptr<logging::Logger> logger);

    ~RPCExecutor() override;

    void start(const std::string& executorRPCAddress,
               const crypto::KeyPair& keyPair,
               const std::string& rpcStorageAddress,
               const std::string& rpcMessengerAddress,
               const std::string& rpcBlockchainAddress,
               const std::string& rpcVMAddress,
               const std::string& logPath,
               uint8_t networkIdentifier);

public:

    void addContract(const ContractKey& key, AddContractRequest&& request) override;

    void addManualCall(const ContractKey& key, ManualCallRequest&& request) override;

    void setAutomaticExecutionsEnabledSince(const ContractKey& contractKey, uint64_t blockHeight) override;

    void addBlockInfo(uint64_t blockHeight, blockchain::Block&& block) override;

    void addBlock(const ContractKey& contractKey, uint64_t height) override;

    void removeContract(const ContractKey& key, RemoveRequest&& request) override;

    void setExecutors(const ContractKey& key, std::map<ExecutorKey, ExecutorInfo>&& executors) override;

    void onEndBatchExecutionPublished(blockchain::PublishedEndBatchExecutionTransactionInfo&& info) override;

    void onEndBatchExecutionSingleTransactionPublished(
            blockchain::PublishedEndBatchExecutionSingleTransactionInfo&& info) override;

    void onEndBatchExecutionFailed(blockchain::FailedEndBatchExecutionTransactionInfo&& info) override;

    void onStorageSynchronizedPublished(blockchain::PublishedSynchronizeSingleTransactionInfo&& info) override;

private:

    void waitForRPCResponse();

    void readMessage();

    void sendMessage(const executor_server::ServerMessage& message);

    void processSuccessfulEndBatchTransactionIsReadyMessage(
            const executor_server::SuccessfulEndBatchTransactionIsReady& message);

    void processUnsuccessfulEndBatchTransactionIsReadyMessage(
            const executor_server::UnsuccessfulEndBatchTransactionIsReady& message);

    void processEndBatchExecutionSingleTransactionIsReadyMessage(
            const executor_server::EndBatchExecutionSingleTransactionIsReady& message);

    void processSynchronizationSingleTransactionIsReadyMessage(
            const executor_server::SynchronizationSingleTransactionIsReady& message);

    void processReleasedTransactionsAreReadyMessage(
            const executor_server::ReleasedTransactionsAreReady& message);
};

}