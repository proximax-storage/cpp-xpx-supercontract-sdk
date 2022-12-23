/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/Identifiers.h"

#include <memory>
#include "Proofs.h"

namespace sirius::contract {

struct CallExecutorParticipation {
    uint64_t m_scConsumed;
    uint64_t m_smConsumed;

    template<class Archive>
    void serialize( Archive& arch ) {
        arch( m_scConsumed );
        arch( m_smConsumed );
    }
};

struct SuccessfulBatchCallInfo {
    bool m_callExecutionSuccess;
    int64_t m_callSandboxSizeDelta;
    int64_t m_callStateSizeDelta;

    template<class Archive>
    void serialize( Archive& arch ) {
        arch( m_callExecutionSuccess );
        arch( m_callSandboxSizeDelta );
        arch( m_callStateSizeDelta );
    }
};

struct CallExecutionInfo {

    CallId m_callId;

    std::optional<SuccessfulBatchCallInfo> m_callExecutionInfo;

    std::vector<CallExecutorParticipation> m_executorsParticipation;
};

struct SuccessfulBatchInfo {
    StorageHash  m_storageHash;
    uint64_t    m_usedStorageSize;
    uint64_t    m_metaFilesSize;
    uint64_t    m_fileStructureSize;

    crypto::CurvePoint m_PoExVerificationInfo;

    template<class Archive>
    void serialize( Archive& arch ) {
        arch( m_storageHash );
        arch( m_usedStorageSize );
        arch( m_metaFilesSize );
        arch( m_fileStructureSize );
        arch( m_PoExVerificationInfo );
    }
};

struct EndBatchExecutionTransactionInfo {
    ContractKey                         m_contractKey;
    uint64_t                            m_batchIndex;

    std::optional<SuccessfulBatchInfo>  m_successfulBatchInfo;
    std::vector<CallExecutionInfo>      m_callsExecutionInfo;

    std::vector<Proofs>                 m_proofs;
    std::vector<ExecutorKey>            m_executorKeys;
    std::vector<Signature>              m_signatures;
};

struct EndBatchExecutionSingleTransactionInfo {
    ContractKey m_contractKey;
    uint64_t    m_batchIndex = 0;
    Proofs      m_proofOfExecution;
};

struct PublishedEndBatchExecutionTransactionInfo {
    ContractKey                 m_contractKey;
    uint64_t                    m_batchIndex;
    bool                        m_batchSuccess;
    Hash256                     m_driveState;
    crypto::CurvePoint          m_PoExVerificationInfo;
    std::vector <ExecutorKey>   m_cosigners;

//    bool isSuccessful() const {
//        return m_success;
//    }
};

struct PublishedEndBatchExecutionSingleTransactionInfo {
    ContractKey m_contractKey;
    uint64_t    m_batchIndex;
};

struct FailedEndBatchExecutionTransactionInfo {
    ContractKey m_contractKey;
    uint64_t    m_batchIndex;
    bool        m_batchSuccess;
};

}