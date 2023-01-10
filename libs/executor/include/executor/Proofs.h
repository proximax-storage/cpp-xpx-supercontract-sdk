#pragma once

/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <crypto/CurvePoint.h>

namespace sirius::contract {

struct TProof {
    sirius::crypto::CurvePoint m_F;
    sirius::crypto::Scalar m_k;

    template<class Archive>
    void serialize(Archive& arch) {
        arch(m_F);
        arch(m_k);
    }
};

struct BatchProof {
    sirius::crypto::CurvePoint m_T;
    sirius::crypto::Scalar m_r;

    template<class Archive>
    void serialize(Archive& arch) {
        arch(m_T);
        arch(m_r);
    }
};

struct Proofs {
    uint64_t m_initialBatch = 0;
    TProof m_tProof;
    BatchProof m_batchProof;

    template<class Archive>
    void serialize(Archive& arch) {
        arch(m_initialBatch);
        arch(m_tProof);
        arch(m_batchProof);
    }
};

}