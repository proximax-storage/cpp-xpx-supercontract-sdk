/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "virtualMachine/RPCVirtualMachine.h"

namespace sirius::contract::vm {

RPCVirtualMachine::RPCVirtualMachine( const SessionId& sessionId,
                                      const StorageObserver& storageObserver,
                                      GlobalEnvironment& environment,
                                      VirtualMachineEventHandler& virtualMachineEventHandler,
                                      const std::string& serverAddress )
                                      : m_sessionId( sessionId )
                                      , m_storageObserver( storageObserver )
                                      , m_environment( environment )
                                      , m_virtualMachineEventHandler( virtualMachineEventHandler )
                                      , m_stub( std::move( supercontractserver::SupercontractServer::NewStub( grpc::CreateChannel(
                                              serverAddress, grpc::InsecureChannelCredentials()))))
                                              , m_completionQueueThread( [this] {
                                                  waitForRPCResponse();
                                              } ) {}

                                              RPCVirtualMachine::~RPCVirtualMachine() {

    ASSERT( isSingleThread(), m_environment.logger() )

    for ( auto&[_, query]: m_pathQueries ) {
        query->terminate();
    }
    m_completionQueue.Shutdown();
    if ( m_completionQueueThread.joinable()) {
        m_completionQueueThread.join();
    }
}

void RPCVirtualMachine::executeCall( const ContractKey& contractKey, const CallRequest& request ) {

    ASSERT( isSingleThread(), m_environment.logger() )

    std::function<void( std::string&& )> call = [=, this, request = request](
            std::string&& callAbsolutePath ) mutable -> void {
        onReceivedCallAbsolutePath( contractKey, std::move( request ), std::move( callAbsolutePath ));
    };
    auto callback = createAsyncQueryHandler<std::string>( std::move(call), [] {}, m_environment );
    m_pathQueries[request.m_callId] = callback;
    m_storageObserver.getAbsolutePath( request.m_file, callback );
}

void RPCVirtualMachine::onReceivedCallAbsolutePath( const ContractKey& contractKey, CallRequest&& request,
                                                    std::string&& callAbsolutePath ) {

    ASSERT( isSingleThread(), m_environment.logger() )

    m_pathQueries.erase( request.m_callId );

    request.m_file = callAbsolutePath;

    supercontractserver::ExecuteRequest rpcRequest;
    rpcRequest.set_contractkey( std::string( contractKey.begin(), contractKey.end()));
    rpcRequest.set_callid( std::string( request.m_callId.begin(), request.m_callId.end()));
    rpcRequest.set_filetocall( std::move( request.m_file ));
    rpcRequest.set_functiontocall( std::move( request.m_function ));
    rpcRequest.set_scprepayment( request.m_scLimit );
    rpcRequest.set_smprepayment( request.m_smLimit );
    rpcRequest.set_contract_mode( (uint32_t) request.m_callLevel );

    auto* call = new ExecuteCallRPCVirtualMachineRequest( rpcRequest, m_virtualMachineEventHandler );

    call->m_response_reader =
            m_stub->PrepareAsyncExecuteCall( &call->m_context, rpcRequest, &m_completionQueue );
    m_stub->PrepareAsyncExecuteCall()

    call->m_response_reader->StartCall();
    call->m_response_reader->Finish( &call->m_reply, &call->m_status, &call );
}

void RPCVirtualMachine::waitForRPCResponse() {

    ASSERT( !isSingleThread(), m_environment.logger() )

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

}