/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "Identifiers.h"

namespace sirius::contract {

struct AddContractRequest {

    // state
    DriveKey                m_driveKey;
    std::set<ExecutorKey>   m_executors;
    uint64_t                m_batchesExecuted;
    uint64_t                m_automaticExecutionsSCLimit;
    uint64_t                m_automaticExecutionsSMLimit;

    // config
    uint64_t                m_unsuccessfulApprovalExpectation;
};

struct CallReferenceInfo {
    std::optional<CallerKey> m_callerKey;
    uint64_t m_blockHeight;
    BlockHash m_blockHash;
    uint64_t m_blockTime;
    uint64_t m_blockGenerationTime;
    std::optional<TransactionHash> m_transactionHash;
};

struct CallRequestParameters {
    ContractKey m_contractKey;
    CallId m_callId;
    std::string m_file;
    std::string m_function;
    std::vector<uint8_t> m_params;
    uint64_t m_scLimit;
    uint64_t m_smLimit;
    CallReferenceInfo m_referenceInfo;
};

struct RemoveRequest {
    Hash256 m_closeId;
};

struct Block{
    BlockHash m_blockHash;
    uint64_t m_height;
};

}