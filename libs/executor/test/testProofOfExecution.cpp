/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "TestUtils.h"
#include "ProofOfExecution.h"
#include "gtest/gtest.h"

namespace sirius::contract::test {

#define TEST_NAME Supercontract

void tProofVerification(blockchain::Proofs p, crypto::KeyPair& key) {
    auto T = p.m_batchProof.m_T;
    auto r = p.m_batchProof.m_r;
    auto F = p.m_tProof.m_F;
    auto k = p.m_tProof.m_k;
    sirius::crypto::Sha3_512_Builder hasher_h;
    Hash512 d_h;
    hasher_h.update({F.toBytes(), T.toBytes(), key.publicKey()});
    hasher_h.final(d_h);
    sirius::crypto::Scalar d(d_h.array());
    auto beta = sirius::crypto::CurvePoint::BasePoint();
    auto expected = k * beta;
    T *= d;
    expected += T;
    ASSERT_EQ(F, expected);
}

void tProofVerificationFalse(blockchain::Proofs p, crypto::KeyPair& key) {
    auto T = p.m_batchProof.m_T;
    auto r = p.m_batchProof.m_r;
    auto F = p.m_tProof.m_F;
    auto k = p.m_tProof.m_k;
    sirius::crypto::Sha3_512_Builder hasher_h;
    Hash512 d_h;
    hasher_h.update({F.toBytes(), T.toBytes(), key.publicKey()});
    hasher_h.final(d_h);
    sirius::crypto::Scalar d(d_h.array());
    auto beta = sirius::crypto::CurvePoint::BasePoint();
    auto expected = k * beta;
    T *= d;
    expected += T;
    ASSERT_NE(F, expected);
}

void batchProofVerification(blockchain::Proofs n, blockchain::Proofs m, sirius::crypto::CurvePoint cY) {
    auto beta = sirius::crypto::CurvePoint::BasePoint();

    auto T_m = m.m_batchProof.m_T;
    auto r_m = m.m_batchProof.m_r;

    auto T_n = n.m_batchProof.m_T;
    auto r_n = n.m_batchProof.m_r;

    auto T_diff = T_n - T_m;
    auto r_diff = r_n - r_m;
    auto rBeta = r_diff * beta;
    auto rhs = rBeta + cY;

    ASSERT_EQ(T_diff, rhs);
};

void batchProofVerificationFalse(blockchain::Proofs n, blockchain::Proofs m, sirius::crypto::CurvePoint cY) {
    auto beta = sirius::crypto::CurvePoint::BasePoint();

    auto T_m = m.m_batchProof.m_T;
    auto r_m = m.m_batchProof.m_r;

    auto T_n = n.m_batchProof.m_T;
    auto r_n = n.m_batchProof.m_r;

    auto T_diff = T_n - T_m;
    auto r_diff = r_n - r_m;
    auto rBeta = r_diff * beta;
    auto rhs = rBeta + cY;

    ASSERT_NE(T_diff, rhs);
};

TEST(TEST_NAME, PoEx) {
    auto key = crypto::KeyPair::FromString("93f068088c13ef63b5aa55822acf75d823965dd997df9e980b273f15891ceddc");
    sirius::contract::ProofOfExecution poex(key);

    auto m = poex.buildActualProof();
    tProofVerification(m, key);

    uint64_t secretInfo[3] = {13561546964161623, 1255621556321561123, 431614452611456511};
    sirius::crypto::CurvePoint cY;
    auto beta = sirius::crypto::CurvePoint::BasePoint();
    for (auto i = 0; i < 3; i++) {
        auto Y = poex.addToProof(secretInfo[i]);

        sirius::crypto::Sha3_512_Builder hasher_h;
        Hash512 cHash;
        hasher_h.update({beta.toBytes(), Y.toBytes(), key.publicKey()});
        hasher_h.final(cHash);
        sirius::crypto::Scalar c(cHash.array());
        auto part = Y * c;
        cY += part;
    }

    auto m2 = poex.buildActualProof();
    tProofVerification(m2, key);

    uint64_t secretInfo2[3] = {354625726501424, 7687354345387, 3546387643};
    sirius::crypto::CurvePoint cY2;
    for (auto i = 0; i < 3; i++) {
        auto Y = poex.addToProof(secretInfo2[i]);

        sirius::crypto::Sha3_512_Builder hasher_h;
        Hash512 cHash;
        hasher_h.update({beta.toBytes(), Y.toBytes(), key.publicKey()});
        hasher_h.final(cHash);
        sirius::crypto::Scalar c(cHash.array());
        auto part = Y * c;
        cY2 += part;
    }
    cY += cY2;
    auto n = poex.buildActualProof();
    tProofVerification(n, key);

    batchProofVerification(n, m, cY);
    batchProofVerification(n, m2, cY2);
}

TEST(TEST_NAME, PoExWrongBatch) {
    auto key = crypto::KeyPair::FromString("93f068088c13ef63b5aa55822acf75d823965dd997df9e980b273f15891ceddc");
    sirius::contract::ProofOfExecution poex(key);

    auto m = poex.buildActualProof();
    tProofVerification(m, key);

    uint64_t secretInfo[3] = {13561546964161623, 1255621556321561123, 431614452611456511};
    sirius::crypto::CurvePoint cY;
    auto beta = sirius::crypto::CurvePoint::BasePoint();
    for (auto i = 0; i < 3; i++) {
        auto Y = poex.addToProof(secretInfo[i]);

        sirius::crypto::Sha3_512_Builder hasher_h;
        Hash512 cHash;
        hasher_h.update({beta.toBytes(), Y.toBytes(), key.publicKey()});
        hasher_h.final(cHash);
        sirius::crypto::Scalar c(cHash.array());
        auto part = Y * c;
        cY += part;
    }

    auto m2 = poex.buildActualProof();
    tProofVerification(m2, key);

    uint64_t secretInfo2[3] = {354625726501424, 7687354345387, 3546387643};
    sirius::crypto::CurvePoint cY2;
    for (auto i = 0; i < 3; i++) {
        auto Y = poex.addToProof(secretInfo2[i]);

        sirius::crypto::Sha3_512_Builder hasher_h;
        Hash512 cHash;
        hasher_h.update({beta.toBytes(), Y.toBytes(), key.publicKey()});
        hasher_h.final(cHash);
        sirius::crypto::Scalar c(cHash.array());
        auto part = Y * c;
        cY2 += part;
    }
    cY += cY2;
    auto n = poex.buildActualProof();
    tProofVerification(n, key);

    batchProofVerificationFalse(n, m, cY2);
    batchProofVerificationFalse(n, m2, cY);
}

TEST(TEST_NAME, PoExPopProof) {
    auto key = crypto::KeyPair::FromString("93f068088c13ef63b5aa55822acf75d823965dd997df9e980b273f15891ceddc");
    sirius::contract::ProofOfExecution poex(key);

    auto m = poex.buildActualProof();
    tProofVerification(m, key);

    uint64_t secretInfo[3] = {13561546964161623, 1255621556321561123, 431614452611456511};
    sirius::crypto::CurvePoint cY;
    auto beta = sirius::crypto::CurvePoint::BasePoint();
    for (auto i = 0; i < 3; i++) {
        auto Y = poex.addToProof(secretInfo[i]);

        sirius::crypto::Sha3_512_Builder hasher_h;
        Hash512 cHash;
        hasher_h.update({beta.toBytes(), Y.toBytes(), key.publicKey()});
        hasher_h.final(cHash);
        sirius::crypto::Scalar c(cHash.array());
        auto part = Y * c;
        cY += part;
    }

    auto m2 = poex.buildActualProof();
    tProofVerification(m2, key);
    batchProofVerification(m2, m, cY);

    poex.addToProof(354625726501424);
    poex.popFromProof();
    auto n = poex.buildActualProof();
    tProofVerification(n, key);

    sirius::crypto::CurvePoint empty;
    batchProofVerification(n, m2, empty);

    batchProofVerification(n, m, cY);
}

TEST(TEST_NAME, PoExNonPopProof) {
    auto key = crypto::KeyPair::FromString("93f068088c13ef63b5aa55822acf75d823965dd997df9e980b273f15891ceddc");
    sirius::contract::ProofOfExecution poex(key);

    auto m = poex.buildActualProof();
    tProofVerification(m, key);

    uint64_t secretInfo[3] = {13561546964161623, 1255621556321561123, 431614452611456511};
    sirius::crypto::CurvePoint cY;
    auto beta = sirius::crypto::CurvePoint::BasePoint();
    for (auto i = 0; i < 3; i++) {
        auto Y = poex.addToProof(secretInfo[i]);

        sirius::crypto::Sha3_512_Builder hasher_h;
        Hash512 cHash;
        hasher_h.update({beta.toBytes(), Y.toBytes(), key.publicKey()});
        hasher_h.final(cHash);
        sirius::crypto::Scalar c(cHash.array());
        auto part = Y * c;
        cY += part;
    }

    auto m2 = poex.buildActualProof();
    tProofVerification(m2, key);
    batchProofVerification(m2, m, cY);

    poex.addToProof(354625726501424);
    // poex.popFromProof();
    auto n = poex.buildActualProof();
    tProofVerification(n, key);

    sirius::crypto::CurvePoint empty;
    batchProofVerificationFalse(n, m2, empty);

    batchProofVerificationFalse(n, m, cY);
}

TEST(TEST_NAME, PoExReset) {
    auto key = crypto::KeyPair::FromString("93f068088c13ef63b5aa55822acf75d823965dd997df9e980b273f15891ceddc");
    sirius::contract::ProofOfExecution poex(key);

    auto m = poex.buildActualProof();
    tProofVerification(m, key);

    uint64_t secretInfo[3] = {13561546964161623, 1255621556321561123, 431614452611456511};
    sirius::crypto::CurvePoint cY;
    auto beta = sirius::crypto::CurvePoint::BasePoint();
    for (auto i = 0; i < 3; i++) {
        auto Y = poex.addToProof(secretInfo[i]);

        sirius::crypto::Sha3_512_Builder hasher_h;
        Hash512 cHash;
        hasher_h.update({beta.toBytes(), Y.toBytes(), key.publicKey()});
        hasher_h.final(cHash);
        sirius::crypto::Scalar c(cHash.array());
        auto part = Y * c;
        cY += part;
    }

    sirius::crypto::CurvePoint empty;
    poex.reset(10);
    auto n = poex.buildActualProof();

    batchProofVerification(n, m, empty);
}

TEST(TEST_NAME, PoExNonReset) {
    auto key = crypto::KeyPair::FromString("93f068088c13ef63b5aa55822acf75d823965dd997df9e980b273f15891ceddc");
    sirius::contract::ProofOfExecution poex(key);

    auto m = poex.buildActualProof();
    tProofVerification(m, key);

    uint64_t secretInfo[3] = {13561546964161623, 1255621556321561123, 431614452611456511};
    sirius::crypto::CurvePoint cY;
    auto beta = sirius::crypto::CurvePoint::BasePoint();
    for (auto i = 0; i < 3; i++) {
        auto Y = poex.addToProof(secretInfo[i]);

        sirius::crypto::Sha3_512_Builder hasher_h;
        Hash512 cHash;
        hasher_h.update({beta.toBytes(), Y.toBytes(), key.publicKey()});
        hasher_h.final(cHash);
        sirius::crypto::Scalar c(cHash.array());
        auto part = Y * c;
        cY += part;
    }

    sirius::crypto::CurvePoint empty;
    // poex.reset();
    auto n = poex.buildActualProof();

    batchProofVerificationFalse(n, m, empty);
}

TEST(TEST_NAME, VerifyCorrectProof) {
    auto key = crypto::KeyPair::FromString("93f068088c13ef63b5aa55822acf75d823965dd997df9e980b273f15891ceddc");
    sirius::contract::ProofOfExecution poex(key);

    ExecutorInfo info;

    uint totalBatches = 1000;
    uint alreadyVerifiedBatches = totalBatches / 2;

    for (uint i = 0; i < alreadyVerifiedBatches; i++) {
        auto verificationInfo = poex.addToProof(i + 123);
        poex.addBatchVerificationInformation(i, verificationInfo);
    }

    {
        auto proof = poex.buildActualProof();
        info.m_initialBatch = proof.m_initialBatch;
        info.m_batchProof = proof.m_batchProof;
        info.m_nextBatchToApprove = alreadyVerifiedBatches;
    }

    for (uint i = alreadyVerifiedBatches; i < totalBatches - 1; i++) {
        auto verificationInfo = poex.addToProof(i + 123);
        poex.addBatchVerificationInformation(i, verificationInfo);
    }

    auto verificationInfo = poex.addToProof(totalBatches - 1 + 123);

    ASSERT_TRUE(poex.verifyProof(key.publicKey().array(),
                                 info,
                                 poex.buildActualProof(),
                                 totalBatches - 1,
                                 verificationInfo));
}

TEST(TEST_NAME, VerifyIncorrectProof) {
    auto key = crypto::KeyPair::FromString("93f068088c13ef63b5aa55822acf75d823965dd997df9e980b273f15891ceddc");
    sirius::contract::ProofOfExecution poex(key);

    ExecutorInfo info;

    uint totalBatches = 1000;
    uint alreadyVerifiedBatches = totalBatches / 2;

    for (uint i = 0; i < alreadyVerifiedBatches; i++) {
        auto verificationInfo = poex.addToProof(i + 123);
        poex.addBatchVerificationInformation(i, verificationInfo);
    }

    {
        auto proof = poex.buildActualProof();
        info.m_initialBatch = proof.m_initialBatch;
        info.m_batchProof = proof.m_batchProof;
        info.m_nextBatchToApprove = alreadyVerifiedBatches;
    }

    for (uint i = alreadyVerifiedBatches; i < totalBatches - 1; i++) {
        auto verificationInfo = poex.addToProof(i + 123);
        poex.addBatchVerificationInformation(i, verificationInfo);
    }

    auto verificationInfo = poex.addToProof(totalBatches - 1 + 123);

    auto finalProof = poex.buildActualProof();
    finalProof.m_batchProof.m_T +=
            crypto::Scalar(std::array<uint8_t, 32>{17}) * crypto::CurvePoint::BasePoint();

    ASSERT_FALSE(poex.verifyProof(key.publicKey().array(),
                                 info,
                                 finalProof,
                                 totalBatches - 1,
                                 verificationInfo));
}

TEST(TEST_NAME, VerifyCorrectProofAfterReset) {
    auto key = crypto::KeyPair::FromString("93f068088c13ef63b5aa55822acf75d823965dd997df9e980b273f15891ceddc");
    sirius::contract::ProofOfExecution poex(key);

    ExecutorInfo info;

    uint totalBatches = 1000;
    uint alreadyVerifiedBatches = totalBatches / 2;
    uint startProofFrom = 3 * totalBatches / 4;

    for (uint i = 0; i < alreadyVerifiedBatches; i++) {
        auto verificationInfo = poex.addToProof(i + 123);
        poex.addBatchVerificationInformation(i, verificationInfo);
    }

    {
        auto proof = poex.buildActualProof();
        info.m_initialBatch = proof.m_initialBatch;
        info.m_batchProof = proof.m_batchProof;
        info.m_nextBatchToApprove = alreadyVerifiedBatches;
    }

    for (uint i = alreadyVerifiedBatches; i < startProofFrom; i++) {
        auto verificationInfo = poex.addToProof(i + 123);
        poex.addBatchVerificationInformation(i, verificationInfo);
    }

    poex.reset(startProofFrom);

    for (uint i = startProofFrom; i < totalBatches - 1; i++) {
        auto verificationInfo = poex.addToProof(i + 123);
        poex.addBatchVerificationInformation(i, verificationInfo);
    }

    auto verificationInfo = poex.addToProof(totalBatches - 1 + 123);

    ASSERT_TRUE(poex.verifyProof(key.publicKey().array(),
                                 info,
                                 poex.buildActualProof(),
                                 totalBatches - 1,
                                 verificationInfo));
}

} // namespace sirius::contract::test