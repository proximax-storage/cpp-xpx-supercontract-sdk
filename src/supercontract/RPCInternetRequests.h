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
        : public RPCCallResponse<internet::Internet::AsyncService, internet::String, internet::ReturnStatus, VirtualMachineInternetQueryHandler> {
public:

    OpenConnectionRPCInternetRequest( const SessionId& sessionId,
                                      internet::Internet::AsyncService* service,
                                      grpc::ServerCompletionQueue* completionQueue,
                                      std::weak_ptr<VirtualMachineQueryHandlersKeeper<VirtualMachineInternetQueryHandler>> handlersExtractor,
                                      const bool& serviceTerminated )
    : RPCCallResponse( sessionId, service, completionQueue, handlersExtractor, serviceTerminated ) {
        addNextToCompletionQueue();
    }

    void process() override {

        // MAIN_THREAD

        if (m_status == ResponseStatus::READY_TO_PROCESS) {
            if ( !m_serviceTerminated ) {
                new OpenConnectionRPCInternetRequest( m_sessionId, m_service, m_completionQueue, m_pWeakHandlersExtractor, m_serviceTerminated );
            }
            auto callId = *reinterpret_cast<const CallId*>(m_request.callid().data());

            std::shared_ptr<VirtualMachineInternetQueryHandler> pHandler;
            if ( auto pHandlersExtractor = m_pWeakHandlersExtractor.lock(); pHandlersExtractor ) {
                pHandler = pHandlersExtractor->getHandler( callId );
            }

            if ( pHandler ) {
                pHandler->openConnection( m_request.str(), [this]( uint64_t connectionId ) {
                    onSuccess( connectionId );
                }, [this] {
                    onFailure();
                } );
            }
            else {
                onFailure();
            }
        } else {
            delete this;
        }
    }

private:

    void onSuccess( uint64_t connectionId ) {
        m_reply.set_success( true );
        m_reply.set_integer( connectionId );
        m_status = ResponseStatus::READY_TO_FINISH;
        m_responder.Finish( m_reply, grpc::Status::OK, this );
    }

    void addNextToCompletionQueue() {
        m_service->RequestOpenConnection(&m_serverContext, &m_request, &m_responder, m_completionQueue, m_completionQueue, this);
    }

};

}