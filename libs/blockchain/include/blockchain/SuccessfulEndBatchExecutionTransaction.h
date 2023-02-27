/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "CallExecutorParticipation.h"

#include <cstdint>
#include <vector>
#include <supercontract/Identifiers.h>
#include <crypto/CurvePoint.h>
#include <executor/Proofs.h>

namespace sirius::contract::blockchain {

struct SuccessfulCallExecutionInfo {
    CallId m_callId;
    bool m_manual;
    uint64_t m_block;
    uint16_t m_callExecutionStatus;
    TransactionHash m_releasedTransaction;
    std::vector<CallExecutorParticipation> m_executorsParticipation;
};

struct SuccessfulBatchInfo {
    StorageHash m_storageHash;
    uint64_t m_usedStorageSize;
    uint64_t m_metaFilesSize;

    crypto::CurvePoint m_PoExVerificationInfo;

    bool operator==(const SuccessfulBatchInfo& info) const {
        if (m_storageHash != info.m_storageHash) {
            return false;
        }
        if (m_usedStorageSize != info.m_usedStorageSize) {
            return false;
        }
        if (m_metaFilesSize != info.m_metaFilesSize) {
            return false;
        }
        if (m_PoExVerificationInfo != info.m_PoExVerificationInfo) {
            return false;
        }
        return true;
    }

    template<class Archive>
    void serialize(Archive& arch) {
        arch(m_storageHash);
        arch(m_usedStorageSize);
        arch(m_metaFilesSize);
        arch(m_PoExVerificationInfo);
    }
};

struct SuccessfulEndBatchExecutionTransactionInfo {
    ContractKey m_contractKey;
    uint64_t m_batchIndex;
    uint64_t m_automaticExecutionsCheckedUpTo;

    SuccessfulBatchInfo m_successfulBatchInfo;
    std::vector<SuccessfulCallExecutionInfo> m_callsExecutionInfo;

    std::vector<Proofs> m_proofs;
    std::vector<ExecutorKey> m_executorKeys;
    std::vector<Signature> m_signatures;
};

}