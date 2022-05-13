/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "RPCVirtualMachine.h"
#include "RPCCall.h"

namespace sirius::contract {

//RPCVirtualMachine::RPCVirtualMachine(
//        const StorageObserver& storageObserver,
//        ThreadManager& threadManager,
//        VirtualMachineEventHandler& virtualMachineEventHandler,
//        const std::string& serverAddress )
//        : m_storageObserver( storageObserver )
//        , m_threadManager( threadManager )
//        , m_virtualMachineEventHandler( virtualMachineEventHandler )
//        , m_stub( supercontractserver::SupercontractServer::NewStub( grpc::CreateChannel(
//                  serverAddress, grpc::InsecureChannelCredentials())))
//        , m_completionQueueThread( [this] {
//            waitForRPCResponse();
//        } ) {}

//RPCVirtualMachine::RPCVirtualMachine(
//        const StorageObserver& storageObserver,
//        ThreadManager& threadManager,
//        VirtualMachineEventHandler& virtualMachineEventHandler,
//        const std::string& serverAddress )
//        : m_storageObserver( storageObserver )
//        , m_threadManager( threadManager )
//        , m_virtualMachineEventHandler( virtualMachineEventHandler )
//        , m_stub( std::move(supercontractserver::SupercontractServer::NewStub( grpc::CreateChannel(
//                  "124", grpc::InsecureChannelCredentials()))))
//        , m_completionQueueThread( [this] {
//            waitForRPCResponse();
//        } ) {}

//RPCVirtualMachine::~RPCVirtualMachine() {
//    m_completionQueue.Shutdown();
//    if ( m_completionQueueThread.joinable()) {
//        m_completionQueueThread.join();
//    }
//}

//void RPCVirtualMachine::executeCall( const ContractKey& contractKey, const CallRequest& request ) {
//    m_storageObserver.getAbsolutePath(request.m_file, [pThisWeak = weak_from_this(), contractKey, request = request] ( std::string&& path ) mutable {
//        if (auto pThis = pThisWeak.lock(); pThis) {
//            pThis->m_threadManager.execute([=, request = std::move(request), path = std::move(path)] () mutable {
//                if (auto pThis = pThisWeak.lock(); pThis) {
//                    pThis->onReceivedCallAbsolutePath(contractKey, std::move(request), std::move(path));
//                }
//            });
//        }
//    });
//}

//void RPCVirtualMachine::onReceivedCallAbsolutePath( const ContractKey& contractKey, CallRequest&& request,
//                                                    std::string&& callAbsolutePath ) {
//    request.m_file = callAbsolutePath;
//
//    supercontractserver::ExecuteRequest rpcRequest;
//    rpcRequest.set_contractkey(std::string(contractKey.begin(), contractKey.end()));
//    rpcRequest.set_callid(std::string(request.m_callId.begin(), request.m_callId.end()));
//    rpcRequest.set_filetocall(std::move(request.m_file));
//    rpcRequest.set_functiontocall(std::move(request.m_function));
//    rpcRequest.set_scprepayment(request.m_scLimit);
//    rpcRequest.set_smprepayment(request.m_smLimit);

//    auto* call = new ExecuteCallRPCVirtualMachineCall(rpcRequest, m_virtualMachineEventHandler);
//
//    call->m_response_reader =
//            m_stub->PrepareAsyncExecuteCall(&call->m_context, rpcRequest, &m_completionQueue);
//
//    call->m_response_reader->StartCall();
//    call->m_response_reader->Finish(&call->m_reply, &call->m_status, &call);
//}

//void RPCVirtualMachine::waitForRPCResponse() {
//    void* pTag;
//    bool ok;
//    while (m_completionQueue.Next(&pTag, &ok)) {
//        auto* pQuery = static_cast<RPCVirtualMachineCall *>(pTag);
//        if (ok) {
//            pQuery->processReply();
//        }
//        delete pQuery;
//    }
//}

}
