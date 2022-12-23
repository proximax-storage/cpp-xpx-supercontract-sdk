#pragma once

/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "supercontract/GlobalEnvironment.h"
#include "supercontract/SingleThread.h"
#include "utils/types.h"
#include <crypto/Hashes.h>
#include <executor/Proofs.h>

namespace sirius::contract {

class ProofOfExecution : private SingleThread {

private:
    GlobalEnvironment& m_environment;

    sirius::crypto::Scalar m_x;
    sirius::crypto::Scalar m_xPrevious;

    const crypto::KeyPair& m_keyPair;

    uint64_t m_initialBatch = 0;

public:
    ProofOfExecution(GlobalEnvironment& environment, const crypto::KeyPair& key);
    sirius::crypto::CurvePoint addToProof(uint64_t digest);
    void popFromProof();
    Proofs buildProof();
    void reset(uint64_t nextBatch);

private:

    sirius::crypto::Scalar generateUniqueRandom(const utils::RawBuffer& dataBuffer);
};

} // namespace sirius::contract