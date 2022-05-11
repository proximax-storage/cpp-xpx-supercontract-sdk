/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <grpcpp/grpcpp.h>

//#include "supercontract_server.pb.h"

#include "supercontract/eventHandlers/VirtualMachineEventHandler.h"

namespace sirius::contract {

class RPCVirtualMachineCall {

public:

    virtual ~RPCVirtualMachineCall() = default;

    virtual void processReply() = 0;

};

template<class TRequest, class TReply>
class BaseRPCVirtualMachineCall : public RPCVirtualMachineCall {

public:

    BaseRPCVirtualMachineCall( const TRequest& request ) : m_request( request ) {}

public:

    TRequest m_request;

    TReply m_reply;

    grpc::ClientContext m_context;

    grpc::Status m_status;

    std::unique_ptr<grpc::ClientAsyncResponseReader<TReply>> m_response_reader;

};

class ExecuteCallRPCVirtualMachineCall
        : public BaseRPCVirtualMachineCall<supercontractserver::ExecuteRequest, supercontractserver::ExecuteReturns> {

private:

    VirtualMachineEventHandler& m_virtualMachineEventHandler;

public:

    explicit ExecuteCallRPCVirtualMachineCall(
            const supercontractserver::ExecuteRequest& request,
            VirtualMachineEventHandler& virtualMachineEventHandler )
            : BaseRPCVirtualMachineCall(request)
            , m_virtualMachineEventHandler( virtualMachineEventHandler ) {}

    void processReply() override {
        if ( m_status.ok() ) {
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