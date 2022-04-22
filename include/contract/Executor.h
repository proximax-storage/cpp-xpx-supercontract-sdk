/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "Requests.h"
#include "ExecutorConfig.h"
#include "StorageBridgeEventHandler.h"
#include "MessengerEventHandler.h"


#include "types.h"

#include <memory>
namespace sirius::contract {

class Executor :
        public StorageBridgeEventHandler,
        public MessengerEventHandler {

public:

    virtual ~Executor() = default;

private:
    virtual void addContract( const ContractKey&, AddContractRequest&& ) = 0;

    virtual void addContractCall( const ContractKey&, CallRequest&& ) = 0;

    virtual void removeContract( const ContractKey&, RemoveRequest&& ) = 0;
};

//std::unique_ptr<Executor> createDefaultExecutor(
//        const crypto::KeyPair&,
//        std::string&&  address,
//        std::string&&  port,
//        std::string&&  storageDirectory,
//        std::string&&  sandboxDirectory,
//        const std::vector<ReplicatorInfo>&  bootstraps,
//        bool           useTcpSocket, // use TCP socket (instead of uTP)
//        ReplicatorEventHandler&,
//        DbgReplicatorEventHandler*  dbgEventHandler = nullptr,
//        const char*    dbgReplicatorName = ""

}