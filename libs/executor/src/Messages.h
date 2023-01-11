/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/Identifiers.h"

#include <memory>

#include <crypto/Signer.h>

#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/optional.hpp>

#include "utils/Serializer.h"
#include <executor/Transactions.h>

namespace sirius::contract {

enum class MessageTag {
    SUCCESSFUL_END_BATCH,
    UNSUCCESSFUL_END_BATCH
};

struct SuccessfulCallExecutionOpinion {

    CallId m_callId;

    SuccessfulBatchCallInfo m_successfulCallExecutionInfo;

    CallExecutorParticipation m_executorParticipation;

    template<class Archive>
    void serialize( Archive& arch ) {
        arch( m_callId );
        arch( m_successfulCallExecutionInfo );
        arch( m_executorParticipation );
    }
};

struct SuccessfulEndBatchExecutionOpinion {
    ContractKey m_contractKey;
    uint64_t m_batchIndex;

    SuccessfulBatchInfo m_successfulBatchInfo;
    std::vector<SuccessfulCallExecutionOpinion> m_callsExecutionInfo;

    Proofs m_proof;

    ExecutorKey m_executorKey;
    Signature m_signature;

    void sign( const crypto::KeyPair& keyPair ) {

        auto signedInfo = getSignedInfo();

        crypto::Sign( keyPair,
                      utils::RawBuffer{signedInfo.data(), signedInfo.size()},
                      m_signature );
    }

    bool verify() const {

        auto signedInfo = getSignedInfo();

        return crypto::Verify( m_executorKey,
                               utils::RawBuffer{signedInfo.data(), signedInfo.size()},
                               m_signature );
    }

    template<class Archive>
    void serialize( Archive& arch ) {
        arch( m_contractKey );
        arch( m_batchIndex );
        arch( m_successfulBatchInfo );
        arch( m_callsExecutionInfo );
//        arch( m_proof );
//        arch( m_executorKey );
//        arch( m_signature );
    }

private:

    std::vector<uint8_t> getSignedInfo() const {
        std::vector<uint8_t> buffer;

        utils::copyToVector( buffer, m_contractKey.data(), Key_Size );
        utils::copyToVector( buffer, reinterpret_cast<const uint8_t*>(m_batchIndex), sizeof( uint64_t ));

        utils::copyToVector( buffer, reinterpret_cast<const uint8_t*>(true), sizeof( bool ));
        utils::copyToVector( buffer, m_successfulBatchInfo.m_storageHash.data(), sizeof(StorageHash));
        utils::copyToVector( buffer, reinterpret_cast<const uint8_t*>(&m_successfulBatchInfo.m_usedStorageSize),
                             sizeof( uint64_t ));
        utils::copyToVector( buffer, reinterpret_cast<const uint8_t*>(&m_successfulBatchInfo.m_metaFilesSize),
                             sizeof( uint64_t ));
        utils::copyToVector( buffer, reinterpret_cast<const uint8_t*>(&m_successfulBatchInfo.m_fileStructureSize),
                             sizeof( uint64_t ));

        for ( const auto& call: m_callsExecutionInfo ) {
            utils::copyToVector( buffer, call.m_callId.data(), sizeof(CallId) );

            auto& successfulCallInfo = call.m_successfulCallExecutionInfo;
            utils::copyToVector( buffer, reinterpret_cast<const uint8_t*>(&successfulCallInfo.m_callExecutionSuccess),
                                 sizeof( bool ));
            utils::copyToVector( buffer, reinterpret_cast<const uint8_t*>(&successfulCallInfo.m_callSandboxSizeDelta),
                                 sizeof( int64_t ));
            utils::copyToVector( buffer, reinterpret_cast<const uint8_t*>(&successfulCallInfo.m_callStateSizeDelta),
                                 sizeof( int64_t ));

            const auto& participation = call.m_executorParticipation;
            utils::copyToVector( buffer, reinterpret_cast<const uint8_t*>(&participation.m_scConsumed), sizeof( uint64_t ));
            utils::copyToVector( buffer, reinterpret_cast<const uint8_t*>(&participation.m_smConsumed), sizeof( uint64_t ));
        }

        return buffer;
    }
};

struct UnsuccessfulEndBatchExecutionOpinion {
    ContractKey m_contractKey;
    uint64_t m_batchIndex;

    std::vector<SuccessfulCallExecutionOpinion> m_callsExecutionInfo;

    Proofs m_proof;

    ExecutorKey m_executorKey;
    Signature m_signature;

    void sign( const crypto::KeyPair& keyPair ) {

        auto signedInfo = getSignedInfo();

        crypto::Sign( keyPair,
                      utils::RawBuffer{signedInfo.data(), signedInfo.size()},
                      m_signature );
    }

    bool verify() const {

        auto signedInfo = getSignedInfo();

        return crypto::Verify( m_executorKey,
                               utils::RawBuffer{signedInfo.data(), signedInfo.size()},
                               m_signature );
    }

    template<class Archive>
    void serialize( Archive& arch ) {
        arch( m_contractKey );
        arch( m_batchIndex );
        arch( m_callsExecutionInfo );
//        arch( m_proof );
//        arch( m_executorKey );
//        arch( m_signature );
    }

private:

    std::vector<uint8_t> getSignedInfo() const {
        std::vector<uint8_t> buffer;

        utils::copyToVector( buffer, m_contractKey.data(), Key_Size );
        utils::copyToVector( buffer, reinterpret_cast<const uint8_t*>(m_batchIndex), sizeof( uint64_t ));

        utils::copyToVector( buffer, reinterpret_cast<const uint8_t*>(false), sizeof( bool ));

        for ( const auto& call: m_callsExecutionInfo ) {
            utils::copyToVector( buffer, call.m_callId.data(), sizeof(CallId) );

            const auto& participation = call.m_executorParticipation;
            utils::copyToVector( buffer, reinterpret_cast<const uint8_t*>(&participation.m_scConsumed), sizeof( uint64_t ));
            utils::copyToVector( buffer, reinterpret_cast<const uint8_t*>(&participation.m_smConsumed), sizeof( uint64_t ));
        }

        return buffer;
    }
};
}