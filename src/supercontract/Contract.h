/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "types.h"

#include "contract/ExecutorEventHandler.h"
#include "contract/Requests.h"
#include "contract/Messenger.h"
#include "contract/StorageBridge.h"
#include "contract/StorageQueries.h"
#include "contract/StorageBridgeEventHandler.h"
#include "eventHandlers/ContractStorageBridgeEventHandler.h"
#include "eventHandlers/ContractVirtualMachineEventHandler.h"
#include "eventHandlers/ContractMessageEventHandler.h"
#include "eventHandlers/ContractBlockchainEventHandler.h"
#include "supercontract/eventHandlers/VirtualMachineEventHandler.h"
#include "VirtualMachine.h"

namespace sirius::contract {

class Contract
        : public ContractStorageBridgeEventHandler,
          public ContractVirtualMachineEventHandler,
          public ContractMessageEventHandler {
public:

    virtual void addContractCall( const CallRequest& ) = 0;

};

std::unique_ptr<Contract> createDefaultContract( const ContractKey&,
                                                 const AddContractRequest&,
                                                 ExecutorEventHandler&,
                                                 Messenger&,
                                                 StorageBridge&,
                                                 VirtualMachine&);

}