/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/ExecutorConfig.h"
#include "supercontract/Identifiers.h"

#include "supercontract/ExecutorEventHandler.h"
#include "supercontract/Requests.h"
#include <messenger/Messenger.h>
#include <storage/Storage.h>
#include "ContractMessageEventHandler.h"
#include "ContractBlockchainEventHandler.h"
#include <virtualMachine/VirtualMachine.h>
#include "ExecutorEnvironment.h"

namespace sirius::contract {

class Contract
          : public ContractMessageEventHandler
          , public ContractBlockchainEventHandler {
public:

    virtual void addManualCall(const CallRequestParameters& ) = 0;

    virtual void setAutomaticExecutionsEnabledSince( const std::optional<uint64_t>& blockHeight ) = 0;

    virtual void addBlockInfo( const Block& ) = 0;

    virtual void removeContract( const RemoveRequest& ) = 0;

    virtual void setExecutors( std::set<ExecutorKey>&& executors ) = 0;

    virtual void terminate() = 0;

};

}