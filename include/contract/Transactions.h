/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/optional.hpp>

#include "types.h"

#include <memory>

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

    Hash256 m_callId;

    std::optional<SuccessfulBatchCallInfo> m_callExecutionInfo;

    std::vector<CallExecutorParticipation> m_executorsParticipation;

    template<class Archive>
    void serialize( Archive& arch ) {
        arch( m_callId );
        arch( m_callExecutionInfo );
        arch( m_executorsParticipation );
    }
};

// Temporary
struct PoExVerificationInfo {
    template<class Archive>
    void serialize( Archive& arch ) {
    }
};

// Temporary
struct PoEx {
    template<class Archive>
    void serialize( Archive& arch ) {
    }
};

struct SuccessfulBatchInfo {
    Hash256  m_rootHash;
    uint64_t m_usedDriveSize;
    uint64_t m_metaFilesSize;
    uint64_t m_fileStructureSize;

    PoExVerificationInfo m_verificationInfo;

    template<class Archive>
    void serialize( Archive& arch ) {
        arch( m_usedDriveSize );
        arch( m_metaFilesSize );
        arch( m_fileStructureSize );
    }
};

struct EndBatchExecutionTransactionInfo {
    ContractKey                         m_contractKey;
    uint64_t                            m_batchIndex;

    std::optional<SuccessfulBatchInfo>  m_successfulBatchInfo;
    std::vector<CallExecutionInfo>      m_callsExecutionInfo;

    std::vector<PoEx>                   m_proofs;
    std::vector<ExecutorKey>            m_executorKeys;
    std::vector<Signature>              m_signatures;

    bool isSuccessful() const {
        return m_successfulBatchInfo.has_value();
    }

    template<class Archive>
    void serialize( Archive& arch ) {
        arch( m_contractKey );
        arch( m_batchIndex );
        arch( m_successfulBatchInfo );
        arch( m_callsExecutionInfo );
        arch( m_proofs );
        arch( m_executorKeys );
        arch( m_signatures );
    }
};

struct EndBatchExecutionSingleTransactionInfo {
    ContractKey m_contractKey;
    uint64_t    m_batchIndex = 0;
    PoEx        m_proofOfExecution;
};

struct PublishedEndBatchExecutionTransactionInfo {
    ContractKey m_contractKey;
    uint64_t    m_batchIndex;

    // Has value only in the case of successful batch
    std::optional<Hash256> m_driveState;

    std::vector<ExecutorKey> m_cosigners;

    bool isSuccessful() const {
        return m_driveState.has_value();
    }
};

struct PublishedEndBatchExecutionSingleTransactionInfo {
    ContractKey m_contractKey;
    uint64_t    m_batchIndex;
};

}