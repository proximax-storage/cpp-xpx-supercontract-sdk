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
#include "supercontract/Messenger.h"
#include "supercontract/Storage.h"
#include "supercontract/StorageQueries.h"
#include "supercontract/StorageEventHandler.h"
#include "ContractStorageEventHandler.h"
#include "ContractVirtualMachineEventHandler.h"
#include "ContractMessageEventHandler.h"
#include "ContractBlockchainEventHandler.h"
#include "VirtualMachineEventHandler.h"
#include "VirtualMachine.h"
#include "ExecutorEnvironment.h"
#include "DebugInfo.h"

namespace sirius::contract {

class Contract
        : public ContractStorageEventHandler,
          public ContractVirtualMachineEventHandler,
          public ContractMessageEventHandler,
          public ContractBlockchainEventHandler {
public:

    virtual void addContractCall( const CallRequest& ) = 0;

    virtual void setAutomaticExecutionsEnabledSince( const std::optional<uint64_t>& blockHeight ) = 0;

    virtual void addBlockInfo( const Block& ) = 0;

    virtual void removeContract( const RemoveRequest& ) = 0;

    virtual void setExecutors( std::set<ExecutorKey>&& executors ) = 0;

    virtual void terminate() = 0;

};

std::unique_ptr<Contract> createDefaultContract( const ContractKey& contractKey,
                                                 AddContractRequest&& addContractRequest,
                                                 ExecutorEnvironment& contractContext,
                                                 const ExecutorConfig& executorConfig,
                                                 const DebugInfo& debugInfo );

}