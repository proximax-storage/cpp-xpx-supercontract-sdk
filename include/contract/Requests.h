/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "types.h"

namespace sirius::contract {

struct AddContractRequest {

    // state
    DriveKey m_driveKey;
    std::set<ExecutorKey> m_executors;
    bool isInActualState;

    // config
    uint64_t m_unsuccessfulApprovalExpectation;
};

struct CallRequest {
    Hash256 m_callId;
};

}