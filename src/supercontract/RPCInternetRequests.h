/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "internet.grpc.pb.h"
#include "RPCCall.h"

namespace sirius::contract {
class OpenConnectionRPCInternetRequest
        : public RPCCallResponse<internet::Internet::AsyncService, internet::String, internet::ReturnStatus> {
public:

    OpenConnectionRPCInternetRequest( internet::Internet::AsyncService* service,
                                      grpc::ServerCompletionQueue* completionQueue ) :
            RPCCallResponse( service, completionQueue )
            {}

    void process() override {
        if (m_status == ResponseStatus::READY_TO_PROCESS) {
            new OpenConnectionRPCInternetRequest( m_service, m_completionQueue );
            m_responder.Finish(m_reply, grpc::Status::OK, this);
            m_status = ResponseStatus::READY_TO_FINISH;
        } else {
            delete this;
        }
    }

protected:

    void addNextToCompletionQueue() override {
        m_service->RequestOpenConnection(&m_serverContext, &m_request, &m_responder, m_completionQueue, m_completionQueue, this);
    }

};

}