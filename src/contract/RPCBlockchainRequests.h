/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "blockchain.grpc.pb.h"
#include "RPCCall.h"
#include "VirtualMachineBlockchainQueryHandler.h"

namespace sirius::contract {

class GetCallerRPCBlockchainRequest
        : public RPCCallResponse<blockchain::Blockchain::AsyncService, blockchain::CallOrientedParams, blockchain::Buffer, VirtualMachineBlockchainQueryHandler> {
public:

    GetCallerRPCBlockchainRequest( const SessionId& sessionId,
                                   blockchain::Blockchain::AsyncService* service,
                                   grpc::ServerCompletionQueue* completionQueue,
                                   std::weak_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineBlockchainQueryHandler>> handlersExtractor,
                                   const bool& serviceTerminated,
                                   const DebugInfo& debugInfo )
    : RPCCallResponse( sessionId, service, completionQueue, handlersExtractor, serviceTerminated, debugInfo ) {
        addNextToCompletionQueue();
    }

    void process() override {

        DBG_MAIN_THREAD_DEPRECATED

        SessionId sessionId( m_request.session_id() );
        if ( m_status == ResponseStatus::READY_TO_PROCESS && sessionId == m_sessionId ) {
            if ( !m_serviceTerminated ) {
                new GetCallerRPCBlockchainRequest( m_sessionId, m_service, m_completionQueue, m_pWeakHandlersExtractor, m_serviceTerminated, m_dbgInfo );
            }
            CallId callId( m_request.callid());

            std::shared_ptr<VirtualMachineBlockchainQueryHandler> pHandler;
            if ( auto pHandlersExtractor = m_pWeakHandlersExtractor.lock(); pHandlersExtractor ) {
                pHandler = pHandlersExtractor->getHandler( callId );
            }

            if ( pHandler ) {
                pHandler->getCaller( [this]( CallerKey callerKey ) {
                    onSuccess( callerKey );
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

    void onSuccess( const CallerKey& callerKey ) {

        DBG_MAIN_THREAD_DEPRECATED

        std::string c = callerKey.toString();
        m_reply.set_buffer( callerKey.toString() );
        m_status = ResponseStatus::READY_TO_FINISH;
        m_responder.Finish( m_reply, grpc::Status::OK, this );
    }

    void addNextToCompletionQueue() {

        DBG_MAIN_THREAD_DEPRECATED

        m_service->RequestGetCaller(&m_serverContext, &m_request, &m_responder, m_completionQueue, m_completionQueue, this);
    }

};

}