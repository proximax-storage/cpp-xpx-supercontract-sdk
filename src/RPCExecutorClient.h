/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "executor.grpc.pb.h"
#include <executor/Executor.h>
#include "RPCTagListener.h"
#include <queue>

namespace sirius::contract::executor {

class RPCExecutorClient : public RPCTagListener {

private:

    ThreadManager m_threadManager;

    std::unique_ptr<executor_server::ExecutorServer::Stub> m_stub;
    grpc::CompletionQueue m_completionQueue;

    grpc::ClientContext m_context;
    std::unique_ptr<grpc::ClientAsyncReaderWriter<executor_server::ClientMessage,
            executor_server::ServerMessage>> m_stream;

    std::thread m_blockingThread;

    std::queue<executor_server::ClientMessage> m_requests;

    bool m_outstandingWrite = false;
    bool m_outstandingRead = false;
    bool m_errorOccurred = false;

    std::shared_ptr<Executor> m_executor;

    std::promise<void> m_promise;

public:

    RPCExecutorClient(const std::string& address);

    ~RPCExecutorClient() override;

    std::future<void> run();

    void sendMessage(executor_server::ClientMessage&& request);

private:

    void onWritten(tl::expected<void, std::error_code>&& res) override;

    void onRead(tl::expected<executor_server::ServerMessage, std::error_code>&& res) override;

    void onStarted(tl::expected<void, std::error_code>&& res) override;

    void onFinished(tl::expected<void, std::error_code>&& res) override;

    void waitForRPCResponse();

    void onErrorOccurred();

    void createWriteTag(const executor_server::ClientMessage& request);

    void createReadTag();

    bool hasOutstandingTags() const;

private:

    void processStartExecutor(const executor_server::StartExecutor& message);

    void processAddContract(const executor_server::AddContract& message);

    void processAddManualCall(const executor_server::AddManualCall& message);

    void processSetAutomaticExecutionsEnabledSince(const executor_server::SetAutomaticExecutionsEnabledSince& message);

    void processAddBlockInfo(const executor_server::AddBlockInfo& message);

    void processAddBlock(const executor_server::AddBlock& message);

    void processRemoveContract(const executor_server::RemoveContract& message);

    void processSetExecutors(const executor_server::SetExecutors& message);

    void
    processPublishedEndBatchExecutionTransaction(const executor_server::PublishedEndBatchExecutionTransaction& message);

    void processPublishedEndBatchExecutionSingleTransaction(
            const executor_server::PublishedEndBatchExecutionSingleTransaction& message);

    void processFailedEndBatchExecution(const executor_server::FailedEndBatchExecution& message);

    void
    processPublishedSynchronizeSingleTransaction(const executor_server::PublishedSynchronizeSingleTransaction& message);

};

}