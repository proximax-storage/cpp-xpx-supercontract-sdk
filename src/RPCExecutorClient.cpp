
/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "RPCExecutorClient.h"
#include "RPCTag.h"

#include "WriteRPCTag.h"
#include "ReadRPCTag.h"
#include "StartRPCTag.h"
#include "FinishRPCTag.h"

namespace sirius::contract::executor {

RPCExecutorClient::RPCExecutorClient(const std::string& address)
        : m_stub(executor_server::ExecutorServer::NewStub(grpc::CreateChannel(
                address, grpc::InsecureChannelCredentials())))
        , m_stream(m_stub->PrepareAsyncRunExecutor(&m_context, &m_completionQueue)) {}

RPCExecutorClient::~RPCExecutorClient() {
    if (m_blockingThread.joinable()) {
        m_blockingThread.join();
    }
}

void RPCExecutorClient::sendMessage(executor_server::RunExecutorRequest&& request) {
    m_threadManager.execute([this, request = std::move(request)] {
        if (!m_errorOccurred && !m_outstandingWrite) {
            createWriteTag(request);
        }
    });
}

std::future<void> RPCExecutorClient::run() {
    auto* pStartTag = new StartRPCTag(*this);
    m_stream->StartCall(pStartTag);
    return m_promise.get_future();
}

void RPCExecutorClient::waitForRPCResponse() {
    void* pTag;
    bool ok;
    while (m_completionQueue.Next(&pTag, &ok)) {
        auto* pQuery = static_cast<RPCTag*>(pTag);
        pQuery->process(ok);
        delete pQuery;
    }
}

void RPCExecutorClient::onWritten(tl::expected<void, std::error_code>&& res) {
    m_threadManager.execute([this, res = std::move(res)] {
        m_outstandingWrite = false;
        if (!res || m_errorOccurred) {
            onErrorOccurred();
            return;
        }

        if (!m_requests.empty()) {
            auto request = std::move(m_requests.front());
            m_requests.pop();
            createWriteTag(request);
        }
    });
}

void RPCExecutorClient::onRead(tl::expected<executor_server::RunExecutorResponse, std::error_code>&& res) {
    m_threadManager.execute([this, res = std::move(res)] {
        m_outstandingRead = false;
        if (!res || m_errorOccurred) {
            onErrorOccurred();
            return;
        }

        const auto& response = *res;

        switch (response.server_message_case()) {

        }

        createReadTag();
    });
}

void RPCExecutorClient::onErrorOccurred() {
    m_errorOccurred = true;

    // This will prevent sending further messages from executor
    m_executor.reset();

    if (!hasOutstandingTags()) {
        auto* pFinishTag = new FinishRPCTag(*this);
        m_stream->Finish(&pFinishTag->m_status, pFinishTag);
    }
}

void RPCExecutorClient::createWriteTag(const executor_server::RunExecutorRequest& request) {
    m_outstandingWrite = true;
    auto* pWriteTag = new WriteRPCTag(*this);
    m_stream->Write(request, pWriteTag);
}

bool RPCExecutorClient::hasOutstandingTags() const {
    return m_outstandingRead || m_outstandingWrite;
}

void RPCExecutorClient::onStarted(tl::expected<void, std::error_code>&& res) {
    m_threadManager.execute([this, res = std::move(res)] {
        if (!res) {
            onErrorOccurred();
            return;
        }

        createReadTag();
    });
}

void RPCExecutorClient::createReadTag() {
    m_outstandingRead = true;
    auto* readTag = new ReadRPCTag(*this);
    m_stream->Read(&readTag->m_response, readTag);
}

void RPCExecutorClient::onFinished(tl::expected<void, std::error_code>&& res) {
    m_completionQueue.Shutdown();
    m_promise.set_value();
}

}