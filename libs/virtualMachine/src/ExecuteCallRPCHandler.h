/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCResponseHandler.h"
#include "RPCTag.h"
#include "supercontract/AsyncQuery.h"
#include "supercontract/Requests.h"
#include "supercontract/SingleThread.h"
#include "supercontract_server.grpc.pb.h"
#include "supercontract_server.pb.h"
#include <virtualMachine/CallRequest.h>
#include <virtualMachine/VirtualMachineBlockchainQueryHandler.h>
#include <virtualMachine/VirtualMachineInternetQueryHandler.h>
#include <virtualMachine/VirtualMachineStorageQueryHandler.h>

namespace sirius::contract::vm {

class ExecuteCallRPCHandler
    : private SingleThread,
      public std::enable_shared_from_this<ExecuteCallRPCHandler> {

private:
    GlobalEnvironment& m_environment;

    CallRequest m_request;

    grpc::ClientContext m_context;

    std::unique_ptr<grpc::ClientAsyncReaderWriter<supercontractserver::Request, supercontractserver::Response>> m_stream;

    std::shared_ptr<AsyncQueryCallback<CallExecutionResult>> m_callback;

    std::weak_ptr<VirtualMachineInternetQueryHandler> m_internetQueryHandler;
    std::weak_ptr<VirtualMachineBlockchainQueryHandler> m_blockchainQueryHandler;
    std::weak_ptr<VirtualMachineStorageQueryHandler> m_storageQueryHandler;

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
        std::weak_ptr<VirtualMachineStorageQueryHandler>&& storageQueryHandler,
        std::shared_ptr<AsyncQueryCallback<CallExecutionResult>>&& callback);

    void start();

    void finish();

private:
    void onStarted(expected<void>&&);

    void writeRequest(supercontractserver::Request&&);

    void onWritten(expected<void>&&);

    void onRead(expected<supercontractserver::Response>&& response);

    void processExecuteCallResponse(const supercontractserver::ExecuteReturns& executeCallResponse);

    void postResponse(expected<CallExecutionResult>&& result);

    void onFinished(grpc::Status&&);

    void processOpenInternetConnection(const supercontractserver::OpenConnection&);

    void processReadInternetConnection(const supercontractserver::ReadConnectionStream&);

    void processCloseInternetConnection(const supercontractserver::CloseConnection&);

    void processOpenFile(const supercontractserver::OpenFile&);

    void processWriteFile(const supercontractserver::WriteFileStream&);

    void processReadFile(const supercontractserver::ReadFileStream&);

    void processFlush(const supercontractserver::Flush&);

    void processCloseFile(const supercontractserver::CloseFile&);

    void processPathExist(const supercontractserver::PathExist&);

    void processIsFile(const supercontractserver::IsFile&);

    void processCreateDir(const supercontractserver::CreateDir&);

    void processMoveFile(const supercontractserver::MoveFile&);

    void processRemoveFile(const supercontractserver::RemoveFile&);

    void processCreateIterator(const supercontractserver::CreateDirIterator&);

    void processHasNext(const supercontractserver::HasNextIterator&);

    void processNext(const supercontractserver::NextDirIterator&);

    void processRemoveFileIterator(const supercontractserver::RemoveDirIterator&);

    void processDestroyIterator(const supercontractserver::DestroyDirIterator&);
};

} // namespace sirius::contract::vm