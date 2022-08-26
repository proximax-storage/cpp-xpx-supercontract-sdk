/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "virtualMachine/RPCCall.h"

#include "supercontract/SingleThread.h"
#include "supercontract/GlobalEnvironment.h"

namespace sirius::contract::vm {

template<class TService, class TRequest, class TReply, class THandler>
class RPCCallResponse
        : protected SingleThread
        , public RPCCall {

protected:

    enum class ResponseStatus {
        READY_TO_PROCESS,
        READY_TO_FINISH
    };

    GlobalEnvironment& m_environment;
    const SessionId m_sessionId;
    ResponseStatus m_status;
    TService* m_service;
    grpc::ServerCompletionQueue* m_completionQueue;
    TRequest m_request;
    TReply m_reply;
    grpc::ServerContext m_serverContext;
    grpc::ServerAsyncResponseWriter<TReply> m_responder;
    std::weak_ptr<VirtualMachineQueryHandlersKeeper<THandler>> m_pWeakHandlersExtractor;
    const bool& m_serviceTerminated;

    void onTerminate() {

        ASSERT(isSingleThread(), m_environment.logger())

        m_status = ResponseStatus::READY_TO_FINISH;
        m_responder.FinishWithError( grpc::Status::CANCELLED, this );
    }

public:

    RPCCallResponse( GlobalEnvironment& environment,
                     const SessionId& sessionId,
                     TService* service,
                     grpc::ServerCompletionQueue* completionQueue,
                     std::weak_ptr<VirtualMachineQueryHandlersKeeper<THandler>> handlersExtractor,
                     const bool& serviceTerminated )
            : m_environment(environment)
            , m_sessionId( sessionId )
            , m_status( ResponseStatus::READY_TO_PROCESS )
            , m_service( service )
            , m_completionQueue( completionQueue )
            , m_responder( &m_serverContext )
            , m_pWeakHandlersExtractor ( handlersExtractor )
            , m_serviceTerminated( serviceTerminated ) {}

};

}