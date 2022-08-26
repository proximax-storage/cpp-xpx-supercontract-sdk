/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <grpcpp/grpcpp.h>
#include <supercontract_server.pb.h>

#include "VirtualMachineQueryHandlersKeeper.h"

#include "VirtualMachineEventHandler.h"

#include "log.h"

namespace sirius::contract::vm {

class RPCCall {

public:

    virtual ~RPCCall() = default;

    virtual void process() = 0;

};

template<class TRequest, class TReply>
class RPCCallRequest : public RPCCall {

public:

    explicit RPCCallRequest( const TRequest& request )
    : m_request( request )
    {}

public:

    TRequest m_request;

    TReply m_reply;

    grpc::ClientContext m_context;

    grpc::Status m_status;

    std::unique_ptr<grpc::ClientAsyncResponseReader<TReply>> m_response_reader;

};

class ExecuteCallRPCVirtualMachineRequest
        : public RPCCallRequest<supercontractserver::VariantRequest, supercontractserver::VariantReturn> {

private:

    VirtualMachineEventHandler& m_virtualMachineEventHandler;

public:

    explicit ExecuteCallRPCVirtualMachineRequest(
            const supercontractserver::VariantRequest& request,
            VirtualMachineEventHandler& virtualMachineEventHandler )
            : RPCCallRequest( request )
            , m_virtualMachineEventHandler( virtualMachineEventHandler )
            {}

    void process() override {

//        DBG_SECONDARY_THREAD

        if ( m_status.ok() ) {
            m_request.variant_case()
            auto contractKey = *reinterpret_cast<const ContractKey*>(m_request.contractkey().data());
            auto callId = *reinterpret_cast<const CallId*>(m_request.callid().data());

            CallExecutionResult executionResult = {
                    callId,
                    m_reply.success(),
                    0,
                    m_reply.scconsumed(),
                    m_reply.smconsumed()
                    // m_proofOfExecution
            };
            m_virtualMachineEventHandler.onSuperContractCallExecuted( contractKey, executionResult );
        }
    }

};

}