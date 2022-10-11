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

namespace sirius::contract
{

    class ProofOfExecution : private SingleThread
    {

    private:
        GlobalEnvironment &m_environment;

        sirius::crypto::Scalar m_x;
        sirius::crypto::Scalar m_xPrevious;

        Hash256 m_publicKey;

        std::tuple<sirius::crypto::CurvePoint, sirius::crypto::Scalar> m_b;
        std::tuple<sirius::crypto::CurvePoint, sirius::crypto::Scalar> m_q;

    public:
        ProofOfExecution(GlobalEnvironment &environment, std::array<uint8_t, 32> &publicKey, int seed);
        void addToProof(u_int64_t digest);
        void popFromProof();
        void buildProof();
        void reset();
        std::tuple<sirius::crypto::CurvePoint, sirius::crypto::Scalar> getBProof() const;
        std::tuple<sirius::crypto::CurvePoint, sirius::crypto::Scalar> getTProof() const;
    };

}