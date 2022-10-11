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

    TEST(TEST_NAME, PoEx)
    {
        GlobalEnvironmentMock environment;
        auto &threadManager = environment.threadManager();
        threadManager.execute([&]
                              {
            std::array<uint8_t, 32> publicKey;
            sirius::contract::ProofOfExecution poex(environment, publicKey, 0);
            poex.buildProof();
            auto b = poex.getBProof();
            auto q = poex.getTProof();
            sirius::crypto::Sha3_256_Builder hasher_h;
            Hash256 d_h;
            hasher_h.update({get<0>(q).tobytes(), get<0>(b).tobytes(), publicKey}); 
            hasher_h.final(d_h);
            sirius::crypto::Scalar d(d_h.array());
            sirius::crypto::CurvePoint beta;
            sirius::crypto::CurvePoint T = get<0>(b);
            sirius::crypto::Scalar k = get<1>(q);
            sirius::crypto::CurvePoint expected = k * beta;
            T *= d;
            expected += T;
            ASSERT_EQ(get<0>(q), expected); });
        threadManager.stop();
    }
}