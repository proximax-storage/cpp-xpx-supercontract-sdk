/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <grpcpp/create_channel.h>
#include "VirtualMachine.h"
#include "supercontract_server.grpc.pb.h"
#include "RPCCall.h"
#include "contract/StorageObserver.h"
#include "contract/ThreadManager.h"
#include "supercontract/eventHandlers/VirtualMachineEventHandler.h"
#include "contract/AsyncQuery.h"

namespace sirius::contract {

class RPCVirtualMachine : public VirtualMachine {

private:

    const StorageObserver& m_storageObserver;

    ThreadManager& m_threadManager;

    VirtualMachineEventHandler& m_virtualMachineEventHandler;

    std::unique_ptr<supercontractserver::SupercontractServer::Stub> m_stub;

    grpc::CompletionQueue m_completionQueue;
    std::thread m_completionQueueThread;

    std::map<CallId, std::shared_ptr<AsyncQuery>> m_pathQueries;

public:

    RPCVirtualMachine(
            const StorageObserver& storageObserver,
            ThreadManager& threadManager,
            VirtualMachineEventHandler& virtualMachineEventHandler,
            const std::string& serverAddress )
            : m_storageObserver( storageObserver )
            , m_threadManager( threadManager )
            , m_virtualMachineEventHandler( virtualMachineEventHandler )
            , m_stub( std::move(supercontractserver::SupercontractServer::NewStub( grpc::CreateChannel(
                      serverAddress, grpc::InsecureChannelCredentials()))))
            , m_completionQueueThread( [this] {
                waitForRPCResponse();
            } ) {}

    ~RPCVirtualMachine() override {
        for ( auto& [_, query]: m_pathQueries ) {
            query->terminate();
        }
        m_completionQueue.Shutdown();
        if ( m_completionQueueThread.joinable()) {
            m_completionQueueThread.join();
        }
    }

    void executeCall( const ContractKey& contractKey, const CallRequest& request ) override {
        std::function<void( std::string&& )> call = [=, this, request = request](
                std::string&& callAbsolutePath ) mutable -> void {
            onReceivedCallAbsolutePath( contractKey, std::move( request ), std::move( callAbsolutePath ));
        };
        auto callback = std::make_shared<AbstractAsyncQuery<std::string>>(call, m_threadManager );
        m_pathQueries[request.m_callId] = callback;
        m_storageObserver.getAbsolutePath( request.m_file, callback );
    }

private:

    void
    onReceivedCallAbsolutePath( const ContractKey& contractKey, CallRequest&& request,
                                std::string&& callAbsolutePath ) {

        m_pathQueries.erase( request.m_callId );

        request.m_file = callAbsolutePath;

        supercontractserver::ExecuteRequest rpcRequest;
        rpcRequest.set_contractkey( std::string( contractKey.begin(), contractKey.end()));
        rpcRequest.set_callid( std::string( request.m_callId.begin(), request.m_callId.end()));
        rpcRequest.set_filetocall( std::move( request.m_file ));
        rpcRequest.set_functiontocall( std::move( request.m_function ));
        rpcRequest.set_scprepayment( request.m_scLimit );
        rpcRequest.set_smprepayment( request.m_smLimit );

        auto* call = new ExecuteCallRPCVirtualMachineRequest( rpcRequest, m_virtualMachineEventHandler );

        call->m_response_reader =
                m_stub->PrepareAsyncExecuteCall( &call->m_context, rpcRequest, &m_completionQueue );

        call->m_response_reader->StartCall();
        call->m_response_reader->Finish( &call->m_reply, &call->m_status, &call );
    }

    void waitForRPCResponse() {
        void* pTag;
        bool ok;
        while ( m_completionQueue.Next( &pTag, &ok )) {
            auto* pQuery = static_cast<RPCCall*>(pTag);
            if ( ok ) {
                pQuery->process();
            }
            delete pQuery;
        }
    }
};

}