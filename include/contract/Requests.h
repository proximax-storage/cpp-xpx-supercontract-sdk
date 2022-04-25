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
    DriveKey                m_driveKey;
    std::set<ExecutorKey>   m_executors;
    bool                    m_isContractDeployed;

    // config
    uint64_t                m_unsuccessfulApprovalExpectation;
};

struct CallRequest {
    CallId m_callId;
    std::string m_file;
    std::string m_function;
    std::vector<uint8_t> m_params;
    uint64_t m_scLimit;
    uint64_t m_smLimit;
    bool m_isAutomatic = false;
};

struct RemoveRequest {
    Hash256 m_closeId;
};

struct Block{
    BlockHash m_blockHash;
    uint64_t m_height;
};

}