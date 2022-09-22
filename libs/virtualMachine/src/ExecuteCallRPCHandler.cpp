/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <virtualMachine/CallExecutionResult.h>
#include "ExecuteCallRPCHandler.h"

#include "OpenConnectionRPCHandler.h"
#include "ReadConnectionRPCHandler.h"
#include "CloseConnectionRPCHandler.h"

#include "ExecuteCallRPCStarter.h"
#include "ExecuteCallRPCReader.h"
#include "ExecuteCallRPCWriter.h"
#include "ExecuteCallRPCFinisher.h"

namespace sirius::contract::vm {

ExecuteCallRPCHandler::ExecuteCallRPCHandler(
        GlobalEnvironment& environment,
        CallRequest&& callRequest,
        supercontractserver::SupercontractServer::Stub& stub,
        grpc::CompletionQueue& completionQueue,
        std::weak_ptr<VirtualMachineInternetQueryHandler>&& internetQueryHandler,
        std::weak_ptr<VirtualMachineBlockchainQueryHandler>&& blockchainQueryHandler,
        std::shared_ptr<AsyncQueryCallback<std::optional<CallExecutionResult>>>&& callback)
        : m_environment(environment)
        , m_request(std::move(callRequest))
        , m_stream(stub.PrepareAsyncExecuteCall(&m_context, &completionQueue))
        , m_callback(std::move(callback))
        , m_internetQueryHandler(std::move(internetQueryHandler))
        , m_blockchainQueryHandler(std::move(blockchainQueryHandler)) {}

void ExecuteCallRPCHandler::start() {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(!m_tagQuery, m_environment.logger())
    ASSERT(!m_responseHandler, m_environment.logger())

    m_environment.logger().info("Call {} start", m_request.m_callId);

    auto[query, callback] = createAsyncQuery<bool>([this](bool&& ok) { onStarted(ok); }, [] {},
                                                   m_environment, true, true);
    m_tagQuery = std::move(query);
    auto* starter = new ExecuteCallRPCStarter(m_environment, callback);
    m_stream->StartCall(starter);
}

void ExecuteCallRPCHandler::onStarted(bool ok) {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_tagQuery, m_environment.logger())
    ASSERT(!m_responseHandler, m_environment.logger())

    m_environment.logger().info("Call {} started. Status: {}", m_request.m_callId, ok);

    m_tagQuery.reset();

    if (!ok) {
        postResponse({});
        return;
    }

    auto* pRpcRequest = new supercontractserver::ExecuteRequest();
    pRpcRequest->set_contract_key(std::string(m_request.m_contractKey.begin(), m_request.m_contractKey.end()));
    pRpcRequest->set_call_id(std::string(m_request.m_callId.begin(), m_request.m_callId.end()));
    pRpcRequest->set_file_to_call(std::move(m_request.m_file));
    pRpcRequest->set_function_to_call(std::move(m_request.m_function));
    pRpcRequest->set_sc_prepayment(m_request.m_scLimit);
    pRpcRequest->set_sm_prepayment(m_request.m_smLimit);
    pRpcRequest->set_call_mode((uint32_t) m_request.m_callLevel);

    supercontractserver::Request requestWrapper;
    requestWrapper.set_allocated_request(pRpcRequest);

    writeRequest(std::move(requestWrapper));
}

void ExecuteCallRPCHandler::onWritten(bool ok) {
    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_tagQuery, m_environment.logger())
    ASSERT(!m_responseHandler, m_environment.logger())

    m_environment.logger().info("Call {} wrote message to vm server. Status: {}", m_request.m_callId, ok);

    m_tagQuery.reset();

    if (!ok) {
        postResponse({});
        return;
    }

    auto [query, callback] = createAsyncQuery<std::optional<supercontractserver::Response>>(
            [this](std::optional<supercontractserver::Response>&& response) { onRead(std::move(response)); },
            [] {}, m_environment, true, true);
    m_tagQuery = std::move(query);
    auto* reader = new ExecuteCallRPCReader(m_environment, callback);
    m_stream->Read(&reader->m_response, reader);
}

void ExecuteCallRPCHandler::writeRequest(supercontractserver::Request&& requestWrapper) {

    ASSERT(isSingleThread(), m_environment.logger());

    m_environment.logger().info("Call {} write message to vm server", m_request.m_callId);

    auto [query, callback] = createAsyncQuery<bool>([this](bool&& ok) { onWritten(ok); }, [] {},
                                                        m_environment, true, true);
    auto* writer = new ExecuteCallRPCWriter(m_environment, callback);
    m_tagQuery = std::move(query);
    m_stream->Write(requestWrapper, writer);
}

void ExecuteCallRPCHandler::processExecuteCallResponse(
        const supercontractserver::ExecuteReturns& executeCallResponse) {

    CallExecutionResult executionResult = {
            executeCallResponse.success(),
            executeCallResponse.return_val(),
            executeCallResponse.sc_consumed(),
            executeCallResponse.sm_consumed()
            // m_proofOfExecution
    };

    postResponse(std::move(executionResult));
}

void ExecuteCallRPCHandler::onRead(std::optional<supercontractserver::Response>&& response) {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_tagQuery, m_environment.logger())
    ASSERT(!m_responseHandler, m_environment.logger())

    if (!response) {
        postResponse({});
        return;
    }

    m_tagQuery.reset();

    switch (response->server_message_case()) {
        case supercontractserver::Response::kReturns: {
            processExecuteCallResponse(response->returns());
            break;
        }
        case supercontractserver::Response::kGetServicePayment: {
            break;
        }
        case supercontractserver::Response::kOpenConnection: {
            processOpenInternetConnection(response->open_connection());
            break;
        }
        case supercontractserver::Response::kReadConnectionStream: {
            processReadInternetConnection(response->read_connection_stream());
            break;
        }
        case supercontractserver::Response::kOpenFile: {
            break;
        }
        case supercontractserver::Response::kWriteFileStream: {
            break;
        }
        case supercontractserver::Response::kFlush: {
            break;
        }
        case supercontractserver::Response::kMoveFile: {
            break;
        }
        case supercontractserver::Response::kRemoveFile: {
            break;
        }
        case supercontractserver::Response::kGetBlockHeight: {
            break;
        }
        case supercontractserver::Response::kGetBlockHash: {
            break;
        }
        case supercontractserver::Response::kGetBlockTime: {
            break;
        }
        case supercontractserver::Response::kGetBlockGenerationTime: {
            break;
        }
        case supercontractserver::Response::kGetTransactionHash: {
            break;
        }
        case supercontractserver::Response::kGetCallerPublicKey: {
            break;
        }
        case supercontractserver::Response::kAddTransaction: {
            break;
        }
        case supercontractserver::Response::kGetTransactionBlockHeight: {
            break;
        }
        case supercontractserver::Response::kGetResponseTransactionHash: {
            break;
        }
        case supercontractserver::Response::kGetTransactionContent: {
            break;
        }
        case supercontractserver::Response::kCloseConnection: {
            processCloseInternetConnection(response->close_connection());
            break;
        }
        case supercontractserver::Response::kReadFileStream: {
            break;
        }
        case supercontractserver::Response::kCloseFile: {
            break;
        }
        case supercontractserver::Response::SERVER_MESSAGE_NOT_SET:
            break;
    }
}

void ExecuteCallRPCHandler::postResponse(std::optional<CallExecutionResult>&& result) {
    m_callback->postReply(std::move(result));
}

void ExecuteCallRPCHandler::finish() {
    auto [query, callback] = createAsyncQuery<grpc::Status>([pThis = shared_from_this()](grpc::Status&& status) {
        pThis->onFinished(std::move(status));
    }, [] {}, m_environment, false, true);
    auto* finisher = new ExecuteCallRPCFinisher(m_environment, callback);
    m_stream->Finish(&finisher->m_status, finisher);
}

void ExecuteCallRPCHandler::onFinished(grpc::Status&& status) {
    ASSERT(isSingleThread(), m_environment.logger())

    if (status.error_code()) {
        m_environment.logger().warn("Finished GRPC Call With Error: {}", status.error_message());
    } else {
        m_environment.logger().info("Finished GRPC Call Without Error: {}");
    }
}

void ExecuteCallRPCHandler::processOpenInternetConnection(const supercontractserver::OpenConnection& request) {
    auto [_, callback] = createAsyncQuery<std::optional<supercontractserver::OpenConnectionReturn>>(
            [this](auto&& res) {

                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res) {
                    postResponse({});
                    return;
                }
                auto* status = new supercontractserver::OpenConnectionReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_open_connection_status(status);
                writeRequest(std::move(requestWrapper));
            }, [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<OpenConnectionRPCHandler>(
            m_environment,
            request,
            m_internetQueryHandler,
            callback);
}

void ExecuteCallRPCHandler::processReadInternetConnection(const supercontractserver::ReadConnectionStream& request) {
    auto [_, callback] = createAsyncQuery<std::optional<supercontractserver::InternetReadBufferReturn>>(
            [this](auto&& res) {

                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res) {
                    postResponse({});
                    return;
                }
                auto* status = new supercontractserver::InternetReadBufferReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_internet_read_buffer(status);
                writeRequest(std::move(requestWrapper));
            }, [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<ReadConnectionRPCHandler>(
            m_environment,
            request,
            m_internetQueryHandler,
            callback);
}

void ExecuteCallRPCHandler::processCloseInternetConnection(const supercontractserver::CloseConnection& request) {
    auto [_, callback] = createAsyncQuery<std::optional<supercontractserver::CloseConnectionReturn>>(
            [this](auto&& res) {

                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res) {
                    postResponse({});
                    return;
                }
                auto* status = new supercontractserver::CloseConnectionReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_close_connection_status(status);
                writeRequest(std::move(requestWrapper));
            }, [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<CloseConnectionRPCHandler>(
            m_environment,
            request,
            m_internetQueryHandler,
            callback);
}

}