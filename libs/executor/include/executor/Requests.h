/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <common/Identifiers.h>
#include "ExecutorInfo.h"

namespace sirius::contract {

struct AddContractRequest {

    // state
    DriveKey                                m_driveKey;
    std::map<ExecutorKey, ExecutorInfo>     m_executors;
    std::map<uint64_t, crypto::CurvePoint>  m_recentBatchesInformation;
    ModificationId                          m_contractDeploymentBaseModificationId;
    std::string                             m_automaticExecutionsFileName;
    std::string                             m_automaticExecutionsFunctionName;
    uint64_t                                m_automaticExecutionsSCLimit;
    uint64_t                                m_automaticExecutionsSMLimit;

    // TODO config
    uint64_t                m_unsuccessfulApprovalExpectation;
};

struct RemoveRequest {
};

}