/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "internet.grpc.pb.h"
#include "RPCCall.h"
#include "VirtualMachineInternetQueryHandler.h"

namespace sirius::contract {

class OpenConnectionRPCInternetRequest
: public RPCCallResponse<internet::Internet::AsyncService, internet::URL, internet::ReturnStatus, VirtualMachineInternetQueryHandler> {
public:

    OpenConnectionRPCInternetRequest( const SessionId& sessionId,
                                      internet::Internet::AsyncService* service,
                                      grpc::ServerCompletionQueue* completionQueue,
                                      std::weak_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineInternetQueryHandler>> handlersExtractor,
                                      const bool& serviceTerminated,
                                      const DebugInfo& debugInfo )
    : RPCCallResponse( sessionId, service, completionQueue, handlersExtractor, serviceTerminated, debugInfo ) {
        addNextToCompletionQueue();
    }

    void process() override {

        DBG_MAIN_THREAD

        SessionId sessionId( m_request.session_id() );
        if ( m_status == ResponseStatus::READY_TO_PROCESS && sessionId == m_sessionId ) {
            if ( !m_serviceTerminated ) {
                new OpenConnectionRPCInternetRequest( m_sessionId, m_service, m_completionQueue, m_pWeakHandlersExtractor, m_serviceTerminated, m_dbgInfo );
            }
            CallId callId( m_request.callid());

            std::shared_ptr<VirtualMachineInternetQueryHandler> pHandler;
            if ( auto pHandlersExtractor = m_pWeakHandlersExtractor.lock(); pHandlersExtractor ) {
                pHandler = pHandlersExtractor->getHandler( callId );
            }

            if ( pHandler ) {
                pHandler->openConnection( m_request.url(), "", [this]( const std::optional<uint64_t>& connectionId ) {
                    onSuccess( connectionId );
                }, [this] {
                    onTerminate();
                } );
            }
            else {
                onTerminate();
            }
        } else {
            if ( m_sessionId != sessionId ) {
                _LOG_WARN( "Incorrect Session Id " << sessionId << " " << m_sessionId );
            }
            delete this;
        }
    }

private:

    void onSuccess( const std::optional<uint64_t>& connectionId ) {

        DBG_MAIN_THREAD

        if ( connectionId.has_value() ) {
            m_reply.set_success( true );
            m_reply.set_identifier( *connectionId );
        }
        else {
            m_reply.set_success(false);
        }
        m_status = ResponseStatus::READY_TO_FINISH;
        m_responder.Finish( m_reply, grpc::Status::OK, this );
    }

    void addNextToCompletionQueue() {

        DBG_MAIN_THREAD

        m_service->RequestOpenConnection(&m_serverContext, &m_request, &m_responder, m_completionQueue, m_completionQueue, this);
    }

};

class ReadConnectionRPCInternetRequest
: public RPCCallResponse<internet::Internet::AsyncService, internet::Identifier, internet::ReturnedReadBuffer, VirtualMachineInternetQueryHandler> {
public:

    ReadConnectionRPCInternetRequest( const SessionId& sessionId,
                                      internet::Internet::AsyncService* service,
                                      grpc::ServerCompletionQueue* completionQueue,
                                      std::weak_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineInternetQueryHandler>> handlersExtractor,
                                      const bool& serviceTerminated,
                                      const DebugInfo& debugInfo )
                                      : RPCCallResponse( sessionId, service, completionQueue, handlersExtractor, serviceTerminated, debugInfo ) {
        addNextToCompletionQueue();
    }

    void process() override {

        DBG_MAIN_THREAD

        SessionId sessionId( m_request.session_id() );
        if ( m_status == ResponseStatus::READY_TO_PROCESS && sessionId == m_sessionId ) {
            if ( !m_serviceTerminated ) {
                new ReadConnectionRPCInternetRequest( m_sessionId, m_service, m_completionQueue, m_pWeakHandlersExtractor, m_serviceTerminated, m_dbgInfo );
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
                _LOG_WARN( "Incorrect Session Id " << sessionId << " " << m_sessionId );
            }
            delete this;
        }
    }

private:

    void onSuccess( std::optional<std::vector<uint8_t>>&& data ) {

        DBG_MAIN_THREAD

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

    void addNextToCompletionQueue() {

        DBG_MAIN_THREAD

        m_service->RequestReadConnectionStream(&m_serverContext, &m_request, &m_responder, m_completionQueue, m_completionQueue, this);
    }

};

class CloseConnectionRPCInternetRequest
        : public RPCCallResponse<internet::Internet::AsyncService, internet::Identifier, internet::Success, VirtualMachineInternetQueryHandler> {
public:

    CloseConnectionRPCInternetRequest( const SessionId& sessionId,
                                      internet::Internet::AsyncService* service,
                                      grpc::ServerCompletionQueue* completionQueue,
                                      std::weak_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineInternetQueryHandler>> handlersExtractor,
                                      const bool& serviceTerminated,
                                      const DebugInfo& debugInfo )
                                      : RPCCallResponse( sessionId, service, completionQueue, handlersExtractor, serviceTerminated, debugInfo ) {
        addNextToCompletionQueue();
    }

    void process() override {

        DBG_MAIN_THREAD

        SessionId sessionId( m_request.session_id() );
        if ( m_status == ResponseStatus::READY_TO_PROCESS && sessionId == m_sessionId ) {
            if ( !m_serviceTerminated ) {
                new CloseConnectionRPCInternetRequest( m_sessionId, m_service, m_completionQueue, m_pWeakHandlersExtractor, m_serviceTerminated, m_dbgInfo );
            }
            CallId callId( m_request.callid());

            std::shared_ptr<VirtualMachineInternetQueryHandler> pHandler;
            if ( auto pHandlersExtractor = m_pWeakHandlersExtractor.lock(); pHandlersExtractor ) {
                pHandler = pHandlersExtractor->getHandler( callId );
            }

            if ( pHandler ) {
                pHandler->close( m_request.identifier(), [this]( bool success ) {
                    onSuccess( success );
                    }, [this] {
                    onTerminate();
                } );
            }
            else {
                onTerminate();
            }
        } else {
            if ( m_sessionId != sessionId ) {
                _LOG_WARN( "Incorrect Session Id " << sessionId << " " << m_sessionId );
            }
            delete this;
        }
    }

private:

    void onSuccess( bool success ) {

        DBG_MAIN_THREAD

        m_reply.set_success(success);
        m_status = ResponseStatus::READY_TO_FINISH;
        m_responder.Finish( m_reply, grpc::Status::OK, this );
    }

    void addNextToCompletionQueue() {

        DBG_MAIN_THREAD

        m_service->RequestCloseConnection(&m_serverContext, &m_request, &m_responder, m_completionQueue, m_completionQueue, this);
    }

};

}