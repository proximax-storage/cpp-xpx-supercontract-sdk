/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ReadConnectionRPCInternetResponce.h"

namespace sirius::contract::vm {

ReadConnectionRPCInternetRequest::ReadConnectionRPCInternetRequest( GlobalEnvironment& environment,
                                                                    const SessionId& sessionId,
                                                                    internet::Internet::AsyncService* service,
                                                                    grpc::ServerCompletionQueue* completionQueue,
                                                                    std::weak_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineInternetQueryHandler>> handlersExtractor,
                                                                    const bool& serviceTerminated )
        : RPCCallResponse( environment, sessionId, service, completionQueue, handlersExtractor, serviceTerminated ) {
    addNextToCompletionQueue();
}

void ReadConnectionRPCInternetRequest::process() {

    ASSERT(isSingleThread(), m_environment.logger())

    SessionId sessionId( m_request.session_id() );
    if ( m_status == ResponseStatus::READY_TO_PROCESS && sessionId == m_sessionId ) {
        if ( !m_serviceTerminated ) {
            new ReadConnectionRPCInternetRequest( m_environment, m_sessionId, m_service, m_completionQueue, m_pWeakHandlersExtractor, m_serviceTerminated );
        }
        CallId callId( m_request.callid());

        std::shared_ptr<VirtualMachineInternetQueryHandler> pHandler;
        if ( auto pHandlersExtractor = m_pWeakHandlersExtractor.lock(); pHandlersExtractor ) {
            pHandler = pHandlersExtractor->getHandler( callId );
        }

        if ( pHandler ) {
            pHandler->read( m_request.identifier(), [this]( std::optional<std::vector<uint8_t>>&& data ) {
                onSuccess( std::move( data ) );
                }, [this] {
                onTerminate();
            } );
        }
        else {
            onTerminate();
        }
    } else {
        if ( m_sessionId != sessionId ) {
            m_environment.logger().warn("Incorrect Session Id {} instead of {}", sessionId, m_sessionId);
        }
        delete this;
    }
}

void ReadConnectionRPCInternetRequest::onSuccess( std::optional<std::vector<uint8_t>>&& data ) {

    ASSERT(isSingleThread(), m_environment.logger())

    if ( data.has_value() ) {
        m_reply.set_success( true );
        std::string buffer( data->begin(), data->end() );
        m_reply.set_buffer( std::move( buffer ) );
    }
    else {
        m_reply.set_success(false);
    }
    m_status = ResponseStatus::READY_TO_FINISH;
    m_responder.Finish( m_reply, grpc::Status::OK, this );
}

void ReadConnectionRPCInternetRequest::addNextToCompletionQueue() {

    ASSERT(isSingleThread(), m_environment.logger())

    m_service->RequestReadConnectionStream(&m_serverContext, &m_request, &m_responder, m_completionQueue, m_completionQueue, this);
}

}