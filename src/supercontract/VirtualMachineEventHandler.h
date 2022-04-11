/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "types.h"

namespace sirius::contract {

struct CallExecutionResult {
    Hash256 m_callId;
    bool m_success;
    uint64_t scConsumed;
    uint64_t smConsumed;
    // m_proofOfExecution
};

class VirtualMachineEventHandler {

public:

    virtual ~VirtualMachineEventHandler() = default;

    virtual void
    onSuperContractCallExecuted( const ContractKey& contractKey, const CallExecutionResult& executionResult ) = 0;

};

}