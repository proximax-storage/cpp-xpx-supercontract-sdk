/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "Requests.h"
#include "ExecutorConfig.h"
#include "MessengerEventHandler.h"
#include "BlockchainEventHandler.h"


#include "Identifiers.h"

#include <memory>
namespace sirius::contract {

class Executor :
        public MessengerEventHandler,
        public BlockchainEventHandler {

public:

    virtual ~Executor() = default;

    virtual void addContract( const ContractKey&, AddContractRequest&& ) = 0;

    virtual void addContractCall( const ContractKey&, CallRequest&& ) = 0;

    virtual void addBlockInfo( const ContractKey&, Block&& ) = 0;

    virtual void removeContract( const ContractKey&, RemoveRequest&& ) = 0;

    virtual void setExecutors( const ContractKey&, std::set<ExecutorKey>&& executors ) = 0;
};

}