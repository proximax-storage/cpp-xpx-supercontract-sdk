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
#include "VirtualMachineEventHandler.h"

namespace sirius::contract {

class Contract {
public:

    virtual ~Contract() = default;

    virtual void addContractCall( const CallRequest& ) = 0;

    virtual void onCallExecuted(const CallExecutionResult& executionResult) = 0;

};

std::unique_ptr<Contract> createDefaultContract( const ContractKey& contractKey,
                                                 const AddContractRequest& addContractRequest,
                                                 ExecutorEventHandler& eventHandler,
                                                 Messenger& messenger,
                                                 StorageBridge& storageBridge );

}