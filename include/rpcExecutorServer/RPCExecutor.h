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

namespace sirius::contract::rpcExecutorServer {

class RPCExecutor: public Executor {

private:

    std::unique_ptr<executor::ExecutorEventHandler> m_eventHandler;

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

    RPCExecutor(const std::string& executorRPCAddress);

    ~RPCExecutor() override;

    void start();

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

    SuccessfulEndBatchTransactionIsReady successful_end_batch_transaction_is_ready = 1;
    UnsuccessfulEndBatchTransactionIsReady unsuccessful_end_batch_transaction_is_ready = 2;
    EndBatchExecutionSingleTransactionIsReady end_batch_single_transaction_is_ready = 3;
    SynchronizationSingleTransactionIsReady synchronization_single_transaction_is_ready = 4;
    ReleasedTransactionsAreReady released_transactions_are_ready = 5;

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