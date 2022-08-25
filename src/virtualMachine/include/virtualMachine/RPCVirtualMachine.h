/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <grpcpp/create_channel.h>
#include "VirtualMachine.h"
#include "supercontract_server.grpc.pb.h"
#include "RPCCall.h"
#include "supercontract/StorageObserver.h"
#include "supercontract/ThreadManager.h"
#include "VirtualMachineEventHandler.h"
#include "supercontract/AsyncQuery.h"

#include "supercontract/SingleThread.h"

namespace sirius::contract::vm {

class RPCVirtualMachine : public VirtualMachine, private SingleThread {

private:

    SessionId m_sessionId;

    const StorageObserver& m_storageObserver;

    GlobalEnvironment& m_environment;

    VirtualMachineEventHandler& m_virtualMachineEventHandler;

    std::unique_ptr<supercontractserver::SupercontractServer::Stub> m_stub;

    grpc::CompletionQueue m_completionQueue;
    std::thread m_completionQueueThread;

    std::map<CallId, std::shared_ptr<AsyncQuery>> m_pathQueries;

public:

    RPCVirtualMachine(
            const SessionId& sessionId,
            const StorageObserver& storageObserver,
            GlobalEnvironment& environment,
            VirtualMachineEventHandler& virtualMachineEventHandler,
            const std::string& serverAddress );

    ~RPCVirtualMachine() override;

    void executeCall( const ContractKey& contractKey, const CallRequest& request ) override;

private:

    void
    onReceivedCallAbsolutePath( const ContractKey& contractKey, CallRequest&& request,
                                std::string&& callAbsolutePath );

    void waitForRPCResponse();
};

}