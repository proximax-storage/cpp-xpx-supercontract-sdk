/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "gtest/gtest.h"
#include "supercontract/ProofOfExecution.h"
#include "TestUtils.h"

namespace sirius::contract::test
{

#define TEST_NAME Supercontract

    void tProofVerification(Proofs p, std::array<uint8_t, 32> publicKey)
    {
        auto T = p.b.T;
        auto r = p.b.r;
        auto F = p.q.F;
        auto k = p.q.k;
        sirius::crypto::Sha3_512_Builder hasher_h;
        Hash512 d_h;
        hasher_h.update({F.tobytes(), T.tobytes(), publicKey});
        hasher_h.final(d_h);
        sirius::crypto::Scalar d(d_h.array());
        auto beta = sirius::crypto::CurvePoint::BasePoint();
        auto expected = k * beta;
        T *= d;
        expected += T;
        ASSERT_EQ(F, expected);
    }

    void batchProofVerification(Proofs n, Proofs m, sirius::crypto::CurvePoint cY)
    {
        auto beta = sirius::crypto::CurvePoint::BasePoint();

        auto T_m = m.b.T;
        auto r_m = m.b.r;

        auto T_n = n.b.T;
        auto r_n = n.b.r;

        auto T_diff = T_n - T_m;
        auto r_diff = r_n - r_m;
        auto rBeta = r_diff * beta;
        auto rhs = rBeta + cY;

        ASSERT_EQ(T_diff, rhs);
    };

    TEST(TEST_NAME, PoEx)
    {
        GlobalEnvironmentMock environment;
        auto &threadManager = environment.threadManager();
        threadManager.execute([&]
                              {
            std::array<uint8_t, 32> publicKey = {184, 250, 143, 132, 33, 57, 17, 65, 124, 25, 21, 253, 69, 10, 249, 252, 33, 5, 215, 81, 76, 47, 150, 29, 221, 22, 161, 101, 16, 252, 247, 11};
            sirius::contract::ProofOfExecution poex(environment, publicKey, 87);
            
            auto m = poex.buildProof();
            tProofVerification(m, publicKey);

            std::array<uint64_t, 3> secretInfo = {13561546964161623, 1255621556321561123, 431614452611456511};
            sirius::crypto::CurvePoint cY;
            auto beta = sirius::crypto::CurvePoint::BasePoint();
            for (auto i = secretInfo.begin(); i != secretInfo.end(); i++) {
                auto Y = poex.addToProof(*i);

                sirius::crypto::Sha3_512_Builder hasher_h;
                Hash512 cHash;
                hasher_h.update({beta.tobytes(), Y.tobytes(), publicKey});
                hasher_h.final(cHash);
                sirius::crypto::Scalar c(cHash.array());
                auto part = Y * c;
                cY += part;
            }

            auto m2 = poex.buildProof();
            tProofVerification(m2, publicKey);

            std::array<uint64_t, 3> secretInfo2 = {354625726501424, 7687354345387, 3546387643};
            sirius::crypto::CurvePoint cY2;
            for (auto i = secretInfo2.begin(); i != secretInfo2.end(); i++) {
                auto Y = poex.addToProof(*i);

                sirius::crypto::Sha3_512_Builder hasher_h;
                Hash512 cHash;
                hasher_h.update({beta.tobytes(), Y.tobytes(), publicKey});
                hasher_h.final(cHash);
                sirius::crypto::Scalar c(cHash.array());
                auto part = Y * c;
                cY2 += part;
            }
            cY += cY2;
            auto n = poex.buildProof();
            tProofVerification(n, publicKey);
            
            batchProofVerification(n, m, cY);
            batchProofVerification(n, m2, cY2); });
        threadManager.stop();
    }
}