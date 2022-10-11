/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "gtest/gtest.h"
#include "supercontract/ProofOfExecution.h"

namespace sirius::contract::test
{

#define TEST_NAME Supercontract

    TEST(TEST_NAME, PoEx)
    {
        GlobalEnvironmentMock environment;
        auto &threadManager = environment.threadManager();
        threadManager.execute([&]
                              { sirius::contract::ProofOfExecution poex(environment); });
        threadManager.stop();
    }

}