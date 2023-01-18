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

struct SuccessfulCallExecutionInfo {
    CallId                                  m_callId;
    bool                                    m_manual;
    uint16_t                                m_callExecutionStatus;
    TransactionHash                         m_releasedTransaction;
    std::vector<CallExecutorParticipation>  m_executorsParticipation;

    template<class Archive>
    void serialize(Archive& arch) {
        arch(m_callId);
        arch(m_manual);
        arch(m_callExecutionStatus);
        arch(m_releasedTransaction);
        arch(m_executorsParticipation);
    }
};

struct UnsuccessfulCallExecutionInfo {

    CallId                                  m_callId;
    bool                                    m_manual;
    std::vector<CallExecutorParticipation>  m_executorsParticipation;

    template<class Archive>
    void serialize(Archive& arch) {
        arch(m_callId);
        arch(m_manual);
        arch(m_executorsParticipation);
    }
};

struct SuccessfulBatchInfo {
    StorageHash  m_storageHash;
    uint64_t     m_usedStorageSize;
    uint64_t     m_metaFilesSize;

    crypto::CurvePoint m_PoExVerificationInfo;

    template<class Archive>
    void serialize( Archive& arch ) {
        arch( m_storageHash );
        arch( m_usedStorageSize );
        arch( m_metaFilesSize );
        arch( m_PoExVerificationInfo );
    }
};

struct SuccessfulEndBatchExecutionTransactionInfo {
    ContractKey                         m_contractKey;
    uint64_t                            m_batchIndex;
    uint64_t                            m_automaticExecutionsCheckedUpTo;

    SuccessfulBatchInfo  m_successfulBatchInfo;
    std::vector<SuccessfulCallExecutionInfo>      m_callsExecutionInfo;

    std::vector<Proofs>                 m_proofs;
    std::vector<ExecutorKey>            m_executorKeys;
    std::vector<Signature>              m_signatures;
};

struct UnsuccessfulEndBatchExecutionTransactionInfo {
    ContractKey                         m_contractKey;
    uint64_t                            m_batchIndex;
    uint64_t                            m_automaticExecutionsCheckedUpTo;

    std::vector<UnsuccessfulCallExecutionInfo>      m_callsExecutionInfo;

    std::vector<Proofs>                 m_proofs;
    std::vector<ExecutorKey>            m_executorKeys;
    std::vector<Signature>              m_signatures;
};

struct EndBatchExecutionSingleTransactionInfo {
    ContractKey m_contractKey;
    uint64_t    m_batchIndex = 0;
    Proofs      m_proofOfExecution;
};

struct SynchronizationSingleTransactionInfo {
    ContractKey m_contractKey;
    uint64_t    m_batchIndex = 0;
};

struct PublishedEndBatchExecutionTransactionInfo {
    ContractKey                 m_contractKey;
    uint64_t                    m_batchIndex;
    bool                        m_batchSuccess;
    Hash256                     m_driveState;
    crypto::CurvePoint          m_PoExVerificationInfo;
    uint64_t                    m_automaticExecutionsCheckedUpTo;
    std::set<ExecutorKey>   	m_cosigners;
};

struct PublishedEndBatchExecutionSingleTransactionInfo {
    ContractKey m_contractKey;
    uint64_t    m_batchIndex;
};

struct PublishedSynchronizeSingleTransactionInfo {
	ContractKey m_contractKey;
	uint64_t    m_batchIndex;
};

struct FailedEndBatchExecutionTransactionInfo {
    ContractKey m_contractKey;
    uint64_t    m_batchIndex;
    bool        m_batchSuccess;
};

}