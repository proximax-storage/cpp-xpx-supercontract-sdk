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

class RPCExecutorClient: public RPCTagListener {

private:

    ThreadManager m_threadManager;

    std::unique_ptr<executor_server::ExecutorServer::Stub> m_stub;
    grpc::CompletionQueue m_completionQueue;

    grpc::ClientContext m_context;
    std::unique_ptr<grpc::ClientAsyncReaderWriter<executor_server::RunExecutorRequest,
            executor_server::RunExecutorResponse>> m_stream;

    std::thread m_blockingThread;

    std::queue<executor_server::RunExecutorRequest> m_requests;

    bool m_outstandingWrite = false;
    bool m_outstandingRead = false;
    bool m_errorOccurred = false;

    std::unique_ptr<Executor> m_executor;

    std::promise<void> m_promise;

public:

    RPCExecutorClient(const std::string& address);

    ~RPCExecutorClient() override;

    std::future<void> run();

    void sendMessage(executor_server::RunExecutorRequest&& request);

private:

    void onWritten(tl::expected<void, std::error_code>&& res) override;

    void onRead(tl::expected<executor_server::RunExecutorResponse, std::error_code>&& res) override;

    void onStarted(tl::expected<void, std::error_code>&& res) override;

    void onFinished(tl::expected<void, std::error_code>&& res) override;

    void waitForRPCResponse();

    void onErrorOccurred();

    void createWriteTag(const executor_server::RunExecutorRequest& request);

    void createReadTag();

    bool hasOutstandingTags() const;
};

}