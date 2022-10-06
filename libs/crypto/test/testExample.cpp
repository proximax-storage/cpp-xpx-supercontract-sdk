/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "gtest/gtest.h"
#include "utils/types.h"
extern "C"
{
#include <external/ref10/ge.h>
}

namespace sirius::contract::test
{

#define TEST_NAME Crypto

    TEST(TEST_NAME, Add)
    {
        sirius::Hash256 h;
        ge_p3 A;

        h[0] = 5;

        ge_scalarmult_base(&A, h.data());

        std::vector<unsigned char> expected;
        expected.resize(256);
        ge_p3_tobytes(expected.data(), &A);

        ge_p3 B;
        sirius::Hash256 h2;
        h2[0] = 3;

        ge_scalarmult_base(&B, h2.data());
        std::vector<unsigned char> b_bytes;
        b_bytes.resize(256);
        ge_p3_tobytes(b_bytes.data(), &B);

        ge_p3 C;
        sirius::Hash256 h3;
        h3[0] = 2;

        ge_scalarmult_base(&C, h3.data());

        ge_cached D;
        ge_p3_to_cached(&D, &B);

        ge_p1p1 E;
        ge_add(&E, &C, &D);

        ge_p3 actual_g;
        ge_p1p1_to_p3(&actual_g, &E);
        std::vector<unsigned char> actual;
        actual.resize(256);
        ge_p3_tobytes(actual.data(), &actual_g);
        ASSERT_EQ(actual, expected);
        ASSERT_NE(b_bytes, expected);
    }

    TEST(TEST_NAME, Subtract)
    {
        sirius::Hash256 h;
        ge_p3 A;

        h[0] = 2;

        ge_scalarmult_base(&A, h.data());

        std::vector<unsigned char> expected;
        expected.resize(256);
        ge_p3_tobytes(expected.data(), &A);

        ge_p3 B;
        sirius::Hash256 h2;
        h2[0] = 1;

        ge_scalarmult_base(&B, h2.data());
        std::vector<unsigned char> b_bytes;
        b_bytes.resize(256);
        ge_p3_tobytes(b_bytes.data(), &B);

        ge_p3 C;
        sirius::Hash256 h3;
        h3[0] = 3;

        ge_scalarmult_base(&C, h3.data());

        ge_cached D;
        ge_p3_to_cached(&D, &B);

        ge_p1p1 E;
        ge_sub(&E, &C, &D);

        ge_p3 actual_g;
        ge_p1p1_to_p3(&actual_g, &E);
        std::vector<unsigned char> actual;
        actual.resize(256);
        ge_p3_tobytes(actual.data(), &actual_g);
        ASSERT_EQ(actual, expected);
        ASSERT_NE(b_bytes, expected);
    }

     TEST(TEST_NAME, DoubleScalar)
     {
         sirius::Hash256 h;
         ge_p3 A;

         h[0] = 80;

         ge_scalarmult_base(&A, h.data());

         ge_p2 temp;
         ge_p3_to_p2(&temp, &A);
         std::vector<unsigned char> expected;
         expected.resize(256);
         ge_tobytes(expected.data(), &temp);

         ge_p3 C;
         sirius::Hash256 h2;
         h2[0] = 7;

         ge_scalarmult_base(&C, h2.data());

         sirius::Hash256 h3;
         h3[0] = 11;

         sirius::Hash256 h4;
         h4[0] = 3;

         ge_p2 E;
         ge_double_scalarmult_vartime(&E, h3.data(), &C, h4.data());

         std::vector<unsigned char> actual;
         actual.resize(256);
         ge_tobytes(actual.data(), &E);
         ASSERT_EQ(actual, expected);
     }
}