/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/Identifiers.h"

namespace sirius::contract {

struct CallExecutionResult {
    CallId      m_callId;
    bool        m_success;
    int32_t     m_return;
    uint64_t    m_scConsumed;
    uint64_t    m_smConsumed;
    // m_proofOfExecution
};

class VirtualMachineEventHandler {

public:

    virtual ~VirtualMachineEventHandler() = default;

    virtual void
    onSuperContractCallExecuted( const ContractKey& contractKey, const CallExecutionResult& executionResult ) = 0;

};

}