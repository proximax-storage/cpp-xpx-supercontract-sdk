/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "CallExecutorParticipation.h"

#include <cstdint>
#include <vector>
#include <common/Identifiers.h>
#include <crypto/CurvePoint.h>
#include <blockchain/Proofs.h>

namespace sirius::contract::blockchain {

struct UnsuccessfulCallExecutionInfo {
    CallId m_callId;
    bool m_manual;
    uint64_t m_block;
    std::vector<CallExecutorParticipation> m_executorsParticipation;
};

struct UnsuccessfulEndBatchExecutionTransactionInfo {
    ContractKey m_contractKey;
    uint64_t m_batchIndex;
    uint64_t m_automaticExecutionsCheckedUpTo;

    std::vector<UnsuccessfulCallExecutionInfo> m_callsExecutionInfo;

    std::vector<Proofs> m_proofs;
    std::vector<ExecutorKey> m_executorKeys;
    std::vector<Signature> m_signatures;
};

}