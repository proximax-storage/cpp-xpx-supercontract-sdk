/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <contract/ExecutorConfig.h>
#include "types.h"

#include "contract/ExecutorEventHandler.h"
#include "contract/Requests.h"
#include "contract/Messenger.h"
#include "contract/Storage.h"
#include "contract/StorageQueries.h"
#include "contract/StorageEventHandler.h"
#include "eventHandlers/ContractStorageEventHandler.h"
#include "eventHandlers/ContractVirtualMachineEventHandler.h"
#include "eventHandlers/ContractMessageEventHandler.h"
#include "eventHandlers/ContractBlockchainEventHandler.h"
#include "supercontract/eventHandlers/VirtualMachineEventHandler.h"
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