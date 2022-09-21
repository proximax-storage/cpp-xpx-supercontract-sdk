///*
//*** Copyright 2021 ProximaX Limited. All rights reserved.
//*** Use of this source code is governed by the Apache 2.0
//*** license that can be found in the LICENSE file.
//*/
//
//#include <virtualMachine/RPCInternetRequests.h>
//
//namespace sirius::contract::vm {
//
//OpenConnectionRPCInternetRequest::OpenConnectionRPCInternetRequest( const SessionId& sessionId,
//                                                                    internet::Internet::AsyncService* service,
//                                                                    grpc::ServerCompletionQueue* completionQueue,
//                                                                    std::weak_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineInternetQueryHandler>> handlersExtractor,
//                                                                    const bool& serviceTerminated )
//        : RPCCallResponse(
//                sessionId, service, completionQueue, handlersExtractor, serviceTerminated ) {
//    addNextToCompletionQueue();
//}
//
//void OpenConnectionRPCInternetRequest::process() {
//
//    DBG_MAIN_THREAD_DEPRECATED
//
//    SessionId sessionId( m_request.session_id() );
//    if ( m_status == ResponseStatus::READY_TO_PROCESS && sessionId == m_sessionId ) {
//        if ( !m_serviceTerminated ) {
//            new OpenConnectionRPCInternetRequest( m_sessionId, m_service, m_completionQueue, m_pWeakHandlersExtractor, m_serviceTerminated );
//        }
//        CallId callId( m_request.callid());
//
//        std::shared_ptr<VirtualMachineInternetQueryHandler> pHandler;
//        if ( auto pHandlersExtractor = m_pWeakHandlersExtractor.lock(); pHandlersExtractor ) {
//            pHandler = pHandlersExtractor->getHandler( callId );
//        }
//
//        if ( pHandler ) {
//            pHandler->openConnection( m_request.url(), [this]( const std::optional<uint64_t>& connectionId ) {
//                onSuccess( connectionId );
//                }, [this] {
//                onTerminate();
//            } );
//        }
//        else {
//            onTerminate();
//        }
//    } else {
//        if ( m_sessionId != sessionId ) {
//            _LOG_WARN( "Incorrect Session Id " << sessionId << " " << m_sessionId );
//        }
//        delete this;
//    }
//}
//
//}