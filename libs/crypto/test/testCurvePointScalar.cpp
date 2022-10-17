/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "gtest/gtest.h"
#include "utils/types.h"
// #include "crypto/Scalar.h"
#include "crypto/CurvePoint.h"
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

    TEST(TEST_NAME, NeutralElement)
    {
        sirius::Hash256 h;
        ge_p3 A;

        h[0] = 80;

        ge_scalarmult_base(&A, h.data());

        std::vector<unsigned char> expected;
        expected.resize(256);
        ge_p3_tobytes(expected.data(), &A);

        ge_cached B;
        ge_p3_to_cached(&B, &A);

        ge_p3 C;
        ge_p3_0(&C);

        ge_p1p1 E;
        ge_add(&E, &C, &B);

        ge_p3 actual_g;
        ge_p1p1_to_p3(&actual_g, &E);
        std::vector<unsigned char> actual;
        actual.resize(256);
        ge_p3_tobytes(actual.data(), &actual_g);
        ASSERT_EQ(actual, expected);
    }

    TEST(TEST_NAME, ScalarAdd)
    {
        sirius::crypto::Scalar a;
        a[0] = 3;
        sirius::crypto::Scalar b;
        b[0] = 2;
        sirius::crypto::Scalar c = a + b;
        sirius::crypto::Scalar expected;
        expected[0] = 5;
        ASSERT_EQ(c, expected);
    }

    TEST(TEST_NAME, ScalarAddLarge)
    {
        sirius::crypto::Scalar a = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar b = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar c = a + b;
        sirius::crypto::Scalar expected = sirius::crypto::Scalar::getLMinusOne();
        expected[0] = 235; // I - 2
        ASSERT_EQ(c, expected);
    }

    TEST(TEST_NAME, ScalarMul)
    {
        sirius::crypto::Scalar a;
        a[0] = 3;
        sirius::crypto::Scalar b;
        b[0] = 2;
        sirius::crypto::Scalar c = a * b;
        sirius::crypto::Scalar expected;
        expected[0] = 6;
        ASSERT_EQ(c, expected);
    }

    TEST(TEST_NAME, ScalarMulLarge)
    {
        sirius::crypto::Scalar a = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar b;
        b[0] = 10;
        sirius::crypto::Scalar c = a * b;
        sirius::crypto::Scalar expected = sirius::crypto::Scalar::getLMinusOne();
        expected[0] = 227; // I - 10
        ASSERT_EQ(c, expected);
    }

    TEST(TEST_NAME, ScalarMulLarge2)
    {
        sirius::crypto::Scalar a = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar b = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar c = a * b;
        sirius::crypto::Scalar expected;
        expected[0] = 1;
        ASSERT_EQ(c, expected);
    }

    TEST(TEST_NAME, ScalarSub)
    {
        sirius::crypto::Scalar a;
        a[0] = 10;
        sirius::crypto::Scalar b;
        b[0] = 6;
        sirius::crypto::Scalar c = a - b;
        sirius::crypto::Scalar expected;
        expected[0] = 4;
        ASSERT_EQ(c, expected);
    }

    TEST(TEST_NAME, ScalarSubNegative)
    {
        sirius::crypto::Scalar a;
        a[0] = 4;
        sirius::crypto::Scalar b;
        b[0] = 6;
        sirius::crypto::Scalar c = a - b;
        sirius::crypto::Scalar expected = sirius::crypto::Scalar::getLMinusOne();
        expected[0] = 235; // I - 2
        ASSERT_EQ(c, expected);
    }

    TEST(TEST_NAME, ScalarSubLarge)
    {
        sirius::crypto::Scalar a = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar b = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar c = a - b;
        sirius::crypto::Scalar expected;
        ASSERT_EQ(c, expected);
    }

    TEST(TEST_NAME, ScalarSubLarge2)
    {
        sirius::crypto::Scalar a = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar b;
        b[0] = 10;
        sirius::crypto::Scalar c = a - b;
        sirius::crypto::Scalar expected = sirius::crypto::Scalar::getLMinusOne();
        expected[0] = 226; // I - 11
        ASSERT_EQ(c, expected);
    }

    TEST(TEST_NAME, ScalarAddInPlace)
    {
        sirius::crypto::Scalar a;
        a[0] = 3;
        sirius::crypto::Scalar b;
        b[0] = 2;
        a += b;
        sirius::crypto::Scalar expected;
        expected[0] = 5;
        ASSERT_EQ(a, expected);
    }

    TEST(TEST_NAME, ScalarAddLargeInPlace)
    {
        sirius::crypto::Scalar a = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar b = sirius::crypto::Scalar::getLMinusOne();
        a += b;
        sirius::crypto::Scalar expected = sirius::crypto::Scalar::getLMinusOne();
        expected[0] = 235; // I - 2
        ASSERT_EQ(a, expected);
    }

    TEST(TEST_NAME, ScalarMulInPlace)
    {
        sirius::crypto::Scalar a;
        a[0] = 3;
        sirius::crypto::Scalar b;
        b[0] = 2;
        a *= b;
        sirius::crypto::Scalar expected;
        expected[0] = 6;
        ASSERT_EQ(a, expected);
    }

    TEST(TEST_NAME, ScalarMulLargeInPlace)
    {
        sirius::crypto::Scalar a = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar b;
        b[0] = 10;
        a *= b;
        sirius::crypto::Scalar expected = sirius::crypto::Scalar::getLMinusOne();
        expected[0] = 227; // I - 10
        ASSERT_EQ(a, expected);
    }

    TEST(TEST_NAME, ScalarMulLargeInPlace2)
    {
        sirius::crypto::Scalar a = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar b = sirius::crypto::Scalar::getLMinusOne();
        a *= b;
        sirius::crypto::Scalar expected;
        expected[0] = 1;
        ASSERT_EQ(a, expected);
    }

    TEST(TEST_NAME, ScalarSubInPlace)
    {
        sirius::crypto::Scalar a;
        a[0] = 10;
        sirius::crypto::Scalar b;
        b[0] = 6;
        a -= b;
        sirius::crypto::Scalar expected;
        expected[0] = 4;
        ASSERT_EQ(a, expected);
    }

    TEST(TEST_NAME, ScalarSubNegativeInPlace)
    {
        sirius::crypto::Scalar a;
        a[0] = 4;
        sirius::crypto::Scalar b;
        b[0] = 6;
        a -= b;
        sirius::crypto::Scalar expected = sirius::crypto::Scalar::getLMinusOne();
        expected[0] = 235; // I - 2
        ASSERT_EQ(a, expected);
    }

    TEST(TEST_NAME, ScalarSubLargeInPlace)
    {
        sirius::crypto::Scalar a = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar b = sirius::crypto::Scalar::getLMinusOne();
        a -= b;
        sirius::crypto::Scalar expected;
        ASSERT_EQ(a, expected);
    }

    TEST(TEST_NAME, ScalarSubLarge2InPlace)
    {
        sirius::crypto::Scalar a = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar b;
        b[0] = 10;
        a -= b;
        sirius::crypto::Scalar expected = sirius::crypto::Scalar::getLMinusOne();
        expected[0] = 226; // I - 11
        ASSERT_EQ(a, expected);
    }

    TEST(TEST_NAME, AddProduct)
    {
        sirius::crypto::Scalar a;
        a[0] = 5;
        sirius::crypto::Scalar b;
        b[0] = 10;
        sirius::crypto::Scalar c;
        c[0] = 6;
        sirius::crypto::Scalar actual = a.addProduct(b, c);
        sirius::crypto::Scalar expected;
        expected[0] = 65;
        ASSERT_EQ(actual, expected);
    }

    TEST(TEST_NAME, AddProductLarge)
    {
        sirius::crypto::Scalar a = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar b = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::Scalar c = sirius::crypto::Scalar::getLMinusOne();
        c[0] = 232; // I - 5
        sirius::crypto::Scalar actual = a.addProduct(b, c);
        sirius::crypto::Scalar expected;
        expected[0] = 4;
        ASSERT_EQ(actual, expected);
    }

    TEST(TEST_NAME, CurvePointAdd)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar two;
        two[0] = 2;
        a *= two;
        auto b = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar three;
        three[0] = 3;
        b *= three;
        sirius::crypto::CurvePoint c = a + b;
        auto expected = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar five;
        five[0] = 5;
        expected *= five;
        ASSERT_EQ(expected, c);
    }

    TEST(TEST_NAME, CurvePointAddLarge)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar I = sirius::crypto::Scalar::getLMinusOne();
        a *= I; // I - 1
        auto b = sirius::crypto::CurvePoint::BasePoint();
        I[0] = 234; // I - 3
        b *= I;
        sirius::crypto::CurvePoint c = a + b;
        auto expected = sirius::crypto::CurvePoint::BasePoint();
        I[0] = 233; // I - 4
        expected *= I;
        ASSERT_EQ(expected, c);
    }

    TEST(TEST_NAME, CurvePointSub)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar two;
        two[0] = 8;
        a *= two;
        auto b = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar three;
        three[0] = 3;
        b *= three;
        sirius::crypto::CurvePoint c = a - b;
        auto expected = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar five;
        five[0] = 5;
        expected *= five;
        ASSERT_EQ(expected, c);
    }

    TEST(TEST_NAME, CurvePointSubLarge)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar l = sirius::crypto::Scalar::getLMinusOne();
        a *= l; // I - 1
        auto b = sirius::crypto::CurvePoint::BasePoint();
        l[0] = 234; // I - 3
        b *= l;
        sirius::crypto::CurvePoint c = a - b;
        auto expected = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar scalar;
        scalar[0] = 2;
        expected *= scalar;
        ASSERT_EQ(expected, c);
    }

    TEST(TEST_NAME, CurvePointAddInPlace)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar two;
        two[0] = 2;
        a *= two;
        auto b = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar three;
        three[0] = 3;
        b *= three;
        a += b;
        auto expected = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar five;
        five[0] = 5;
        expected *= five;
        ASSERT_EQ(expected, a);
    }

    TEST(TEST_NAME, CurvePointAddLargeInPlace)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar I = sirius::crypto::Scalar::getLMinusOne();
        a *= I; // I - 1
        auto b = sirius::crypto::CurvePoint::BasePoint();
        I[0] = 234; // I - 3
        b *= I;
        a += b;
        auto expected = sirius::crypto::CurvePoint::BasePoint();
        I[0] = 233; // I - 4
        expected *= I;
        ASSERT_EQ(expected, a);
    }

    TEST(TEST_NAME, CurvePointSubInPlace)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar two;
        two[0] = 8;
        a *= two;
        auto b = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar three;
        three[0] = 3;
        b *= three;
        a -= b;
        auto expected = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar five;
        five[0] = 5;
        expected *= five;
        ASSERT_EQ(expected, a);
    }

    TEST(TEST_NAME, CurvePointSubLargeInPlace)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar l = sirius::crypto::Scalar::getLMinusOne();
        a *= l; // I - 1
        auto b = sirius::crypto::CurvePoint::BasePoint();
        l[0] = 234; // I - 3
        b *= l;
        a -= b;
        auto expected = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar scalar;
        scalar[0] = 2;
        expected *= scalar;
        ASSERT_EQ(expected, a);
    }

    TEST(TEST_NAME, CurvePointMul)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar a_scalar;
        a_scalar[0] = 3;
        sirius::crypto::CurvePoint b = a * a_scalar;
        a *= a_scalar;
        ASSERT_EQ(a, b);
    }

    TEST(TEST_NAME, CurvePointMulDouble)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar a_scalar;
        a_scalar[0] = 3;
        sirius::crypto::Scalar b_scalar;
        b_scalar[0] = 6;
        a *= a_scalar;
        a *= b_scalar;
        auto expected = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar expected_scalar;
        expected_scalar[0] = 18;
        expected *= expected_scalar;
        ASSERT_EQ(expected, a);
    }

    TEST(TEST_NAME, CurvePointMulInvert)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar a_scalar;
        a_scalar[0] = 3;
        sirius::crypto::CurvePoint b = a_scalar * a;
        a *= a_scalar;
        ASSERT_EQ(a, b);
    }

    TEST(TEST_NAME, CurvePointMulLarge)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar I = sirius::crypto::Scalar::getLMinusOne();
        sirius::crypto::CurvePoint b = a * I;
        a *= I;
        ASSERT_EQ(a, b);
    }

    TEST(TEST_NAME, CurvePointNeutralElement)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar two;
        two[0] = 2;
        a *= two;
        sirius::crypto::CurvePoint base;
        sirius::crypto::CurvePoint c = a + base;
        auto expected = sirius::crypto::CurvePoint::BasePoint();
        expected *= two;
        ASSERT_EQ(expected, c);
    }

    TEST(TEST_NAME, NegativeScalar)
    {
        sirius::crypto::Scalar a;
        a[0] = 5;
        auto expected = sirius::crypto::Scalar::getLMinusOne();
        expected[0] -= 4;
        ASSERT_EQ(-a, expected);
    }

    TEST(TEST_NAME, NegativeScalarBig)
    {
        auto a = sirius::crypto::Scalar::getLMinusOne();
        a[0] -= 5;
        sirius::crypto::Scalar expected;
        expected[0] = 6;
        ASSERT_EQ(-a, expected);
    }

    TEST(TEST_NAME, NegativeCurvePoint)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar six;
        six[0] = 6;
        a *= six;
        auto expected = sirius::crypto::CurvePoint::BasePoint();
        auto negFive = sirius::crypto::Scalar::getLMinusOne();
        negFive[0] -= 5;
        expected *= negFive;
        ASSERT_EQ(-a, expected);
    }

    TEST(TEST_NAME, NegativeCurvePointLarge)
    {
        auto a = sirius::crypto::CurvePoint::BasePoint();
        auto neg = sirius::crypto::Scalar::getLMinusOne();
        neg[0] -= 15;
        a *= neg;
        auto expected = sirius::crypto::CurvePoint::BasePoint();
        sirius::crypto::Scalar num;
        num[0] = 16;
        expected *= num;
        ASSERT_EQ(-a, expected);
    }
}