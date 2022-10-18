#pragma once

/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <crypto/CurvePoint.h>
#include <crypto/Hashes.h>
#include "utils/types.h"
#include "supercontract/SingleThread.h"
#include "supercontract/GlobalEnvironment.h"

namespace sirius::contract {

struct TProof {
    sirius::crypto::CurvePoint m_F;
    sirius::crypto::Scalar m_k;
};

struct BatchProof {
    sirius::crypto::CurvePoint m_T;
    sirius::crypto::Scalar m_r;
};

struct Proofs {
    TProof m_tProof;
    BatchProof m_batchProof;
};

class ProofOfExecution : private SingleThread {

private:
    GlobalEnvironment& m_environment;

    sirius::crypto::Scalar m_x;
    sirius::crypto::Scalar m_xPrevious;

    const crypto::KeyPair& m_keyPair;

public:
    ProofOfExecution(GlobalEnvironment& environment, const crypto::KeyPair& key);
    sirius::crypto::CurvePoint addToProof(uint64_t digest);
    void popFromProof();
    Proofs buildProof();
    void reset();
};

}