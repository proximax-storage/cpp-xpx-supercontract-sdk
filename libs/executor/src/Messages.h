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
    bool m_manual = false;
    uint16_t m_callExecutionStatus = 0;
    TransactionHash m_releasedTransaction;
    CallExecutorParticipation m_executorParticipation;

    template<class Archive>
    void serialize(Archive& arch) {
        arch(m_callId);
        arch(m_manual);
        arch(m_callExecutionStatus);
        arch(m_releasedTransaction);
        arch(m_executorParticipation);
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

    void sign(const crypto::KeyPair& keyPair) {

        auto signedInfo = getSignedInfo();

        crypto::Sign(keyPair,
                     utils::RawBuffer{signedInfo.data(), signedInfo.size()},
                     m_signature);
    }

    bool verify() const {

        auto signedInfo = getSignedInfo();

        return crypto::Verify(m_executorKey,
                              utils::RawBuffer{signedInfo.data(), signedInfo.size()},
                              m_signature);
    }

    template<class Archive>
    void serialize(Archive& arch) {
        arch(m_contractKey);
        arch(m_batchIndex);
        arch(m_successfulBatchInfo);
        arch(m_callsExecutionInfo);
        arch(m_proof);
        arch(m_executorKey);
        arch(cereal::binary_data(m_signature.data(), m_signature.size()));
    }

private:

    std::vector<uint8_t> getSignedInfo() const {

        std::ostringstream os;

        os << utils::serialize(m_contractKey);
        os << utils::serialize(m_batchIndex);
        os << utils::serialize(m_successfulBatchInfo.m_storageHash);
        os << utils::serialize(m_successfulBatchInfo.m_usedStorageSize);
        os << utils::serialize(m_successfulBatchInfo.m_metaFilesSize);
        os << utils::serialize(m_successfulBatchInfo.m_PoExVerificationInfo);

        for (const auto& call: m_callsExecutionInfo) {
            os << utils::serialize(call.m_callId);
            os << utils::serialize(call.m_manual);
            os << utils::serialize(call.m_callExecutionStatus);
            os << utils::serialize(call.m_releasedTransaction);
        }

        os << utils::serialize(m_proof);

        for (const auto& call: m_callsExecutionInfo) {
            os << utils::serialize(call.m_executorParticipation);
        }

        auto s = os.str();
        return {s.begin(), s.end()};
    }
};

struct UnsuccessfulCallExecutionOpinion {

    CallId m_callId;
    bool m_manual;
    CallExecutorParticipation m_executorParticipation;

    template<class Archive>
    void serialize(Archive& arch) {
        arch(m_callId);
        arch(m_manual);
        arch(m_executorParticipation);
    }
};

struct UnsuccessfulEndBatchExecutionOpinion {
    ContractKey m_contractKey;
    uint64_t m_batchIndex;

    std::vector<UnsuccessfulCallExecutionOpinion> m_callsExecutionInfo;

    Proofs m_proof;

    ExecutorKey m_executorKey;
    Signature m_signature;

    void sign(const crypto::KeyPair& keyPair) {

        auto signedInfo = getSignedInfo();

        crypto::Sign(keyPair,
                     utils::RawBuffer{signedInfo.data(), signedInfo.size()},
                     m_signature);
    }

    bool verify() const {

        auto signedInfo = getSignedInfo();

        return crypto::Verify(m_executorKey,
                              utils::RawBuffer{signedInfo.data(), signedInfo.size()},
                              m_signature);
    }

    template<class Archive>
    void serialize(Archive& arch) {
        arch(m_contractKey);
        arch(m_batchIndex);
        arch(m_callsExecutionInfo);
        arch(m_proof);
        arch(m_executorKey);
        arch(cereal::binary_data(m_signature.data(), m_signature.size()));
    }

private:

    std::vector<uint8_t> getSignedInfo() const {


        std::ostringstream os;

        os << utils::serialize(m_contractKey);
        os << utils::serialize(m_batchIndex);

        for (const auto& call: m_callsExecutionInfo) {
            os << utils::serialize(call.m_callId);
            os << utils::serialize(call.m_manual);
        }

        os << utils::serialize(m_proof);

        for (const auto& call: m_callsExecutionInfo) {
            os << utils::serialize(call.m_executorParticipation);
        }

        auto s = os.str();
        return {s.begin(), s.end()};
    }
};
}