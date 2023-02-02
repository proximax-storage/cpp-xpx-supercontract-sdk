/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "utils/types.h"
#include <crypto/Hashes.h>
#include <executor/Proofs.h>
#include <executor/ExecutorInfo.h>
#include <supercontract/Identifiers.h>

namespace sirius::contract {

class ProofOfExecution {

private:

    sirius::crypto::Scalar m_x;
    sirius::crypto::Scalar m_xPrevious;

    const crypto::KeyPair& m_keyPair;

    std::map<uint64_t, crypto::CurvePoint> m_batchesVerificationInformation;

    uint64_t m_initialBatch = 0;

    uint64_t m_maxBatchesHistorySize;

public:
    ProofOfExecution(const crypto::KeyPair& key, uint64_t maxBatchesHistorySize=UINT64_MAX);
    sirius::crypto::CurvePoint addToProof(uint64_t digest);
    void popFromProof();
    Proofs buildActualProof();
    Proofs buildPreviousProof();
    void reset(uint64_t nextBatch);
    bool verifyProof(const ExecutorKey& executorKey,
                     const ExecutorInfo& executorInfo,
                     const Proofs& proof,
                     uint64_t batchId,
                     const crypto::CurvePoint& verificationInformation);
    void addBatchVerificationInformation(uint64_t batchId, const crypto::CurvePoint& batchVerificationInformation);

    static std::pair<crypto::Scalar, crypto::CurvePoint> verificationInfo(uint64_t digest);

private:

    Proofs buildProof(const crypto::Scalar& x);

private:

    sirius::crypto::Scalar generateUniqueRandom(const utils::RawBuffer& dataBuffer);
};

} // namespace sirius::contract