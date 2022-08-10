/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "utils/types.h"
#include "VirtualMachineEventHandler.h"
#include "ContractVirtualMachineQueryHandler.h"


namespace sirius::contract {

class ContractVirtualMachineEventHandler : public ContractVirtualMachineQueryHandler {

public:

    virtual bool
    onSuperContractCallExecuted( const CallExecutionResult& executionResult ) {
        return false;
    }

};

}