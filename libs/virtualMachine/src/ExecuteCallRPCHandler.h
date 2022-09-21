/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract_server.grpc.pb.h"
#include "RPCTag.h"
#include "supercontract_server.pb.h"
#include "supercontract/AsyncQuery.h"
#include <virtualMachine/VirtualMachineInternetQueryHandler.h>
#include <virtualMachine/VirtualMachineBlockchainQueryHandler.h>
#include <virtualMachine/CallExecutionResult.h>
#include "supercontract/Requests.h"
#include "supercontract/SingleThread.h"
#include "RPCResponseHandler.h"

namespace sirius::contract::vm {

class ExecuteCallRPCHandler
        : private SingleThread
        , public std::enable_shared_from_this<ExecuteCallRPCHandler> {

private:

    GlobalEnvironment& m_environment;

    CallRequest m_request;

    grpc::ClientContext m_context;

    std::unique_ptr<grpc::ClientAsyncReaderWriter<supercontractserver::Request, supercontractserver::Response>> m_stream;

    std::shared_ptr<AsyncQueryCallback<std::optional<CallExecutionResult>>> m_callback;

    std::weak_ptr<VirtualMachineInternetQueryHandler>   m_internetQueryHandler;
    std::weak_ptr<VirtualMachineBlockchainQueryHandler> m_blockchainQueryHandler;

    std::unique_ptr<RPCResponseHandler> m_responseHandler;

    std::shared_ptr<AsyncQuery> m_tagQuery;

public:

    explicit ExecuteCallRPCHandler(
            GlobalEnvironment& environment,
            CallRequest&& callRequest,
            supercontractserver::SupercontractServer::Stub& stub,
            grpc::CompletionQueue& completionQueue,
            std::weak_ptr<VirtualMachineInternetQueryHandler>&& internetQueryHandler,
            std::weak_ptr<VirtualMachineBlockchainQueryHandler>&& blockchainQueryHandler,
            std::shared_ptr<AsyncQueryCallback<std::optional<CallExecutionResult>>>&& callback );

    void start();

    void finish();

private:

    void onStarted(bool ok);

    void writeRequest( supercontractserver::Request&& );

    void onWritten(bool ok);

    void onRead(std::optional<supercontractserver::Response>&& response);

    void processExecuteCallResponse(const supercontractserver::ExecuteReturns& executeCallResponse);

    void postResponse( std::optional<CallExecutionResult>&& executionResult );

    void onFinished(grpc::Status&&);

    void processOpenInternetConnection(const supercontractserver::OpenConnection&);

    void processReadInternetConnection(const supercontractserver::ReadConnectionStream&);

    void processCloseInternetConnection(const supercontractserver::CloseConnection&);

};

}