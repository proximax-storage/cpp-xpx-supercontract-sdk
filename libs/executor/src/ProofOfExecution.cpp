#include "ProofOfExecution.h"
#include "crypto/PrivateKey.h"
#include "utils/Random.h"

extern "C" {
#include <external/ref10/sc.h>
}

namespace sirius::contract {

ProofOfExecution::ProofOfExecution(const crypto::KeyPair& key,
                                   uint64_t maxBatchesHistorySize)
        : m_keyPair(key)
          , m_maxBatchesHistorySize(maxBatchesHistorySize) {}

sirius::crypto::Scalar ProofOfExecution::generateUniqueRandom(const utils::RawBuffer& dataBuffer) {

    Hash512 privHash;
    sirius::crypto::Sha3_512({m_keyPair.privateKey().data(), m_keyPair.privateKey().size()}, privHash);

    Hash512 h;
    sirius::crypto::Sha3_512_Builder hasher_r;
    hasher_r.update({privHash.data() + Hash512_Size / 2, Hash512_Size / 2});
    hasher_r.update(dataBuffer);
    hasher_r.final(h);
    sirius::crypto::Scalar scalar(h.array());
    return scalar;
}

sirius::crypto::CurvePoint ProofOfExecution::addToProof(uint64_t digest) {
    auto [alpha, Y] = verificationInfo(digest);

    auto Beta = crypto::CurvePoint::BasePoint();

    sirius::crypto::Sha3_512_Builder hasher_h2;
    Hash512 c;
    hasher_h2.update({Beta.toBytes(), Y.toBytes(), m_keyPair.publicKey()});
    hasher_h2.final(c);
    sirius::crypto::Scalar c_scalar(c.array());

    m_xPrevious = m_x;
    m_x += c_scalar * alpha;

    return Y;
}

void ProofOfExecution::popFromProof() {
    m_x = m_xPrevious;
}

blockchain::Proofs ProofOfExecution::buildActualProof() {
    return buildProof(m_x);
}

blockchain::Proofs ProofOfExecution::buildPreviousProof() {
    return buildProof(m_xPrevious);
}

blockchain::Proofs ProofOfExecution::buildProof(const crypto::Scalar& x) {
    auto v = generateUniqueRandom(x);
    auto Beta = sirius::crypto::CurvePoint::BasePoint();
    auto T = v * Beta;
    auto r = v - x;
    blockchain::BatchProof b{T, r};

    auto w = generateUniqueRandom(v);
    auto F = w * Beta;

    Hash512 d_hash;
    sirius::crypto::Sha3_512_Builder hasher_h;
    hasher_h.update({F.toBytes(), T.toBytes(), m_keyPair.publicKey()});
    hasher_h.final(d_hash);

    sirius::crypto::Scalar d(d_hash.array());
    auto k = w - d * v;

    blockchain::TProof q{F, k};
    return blockchain::Proofs{m_initialBatch, q, b};
}

void ProofOfExecution::reset(uint64_t nextBatch) {
    m_x = sirius::crypto::Scalar();
    m_xPrevious = sirius::crypto::Scalar();
    m_initialBatch = nextBatch;
}

void ProofOfExecution::addBatchVerificationInformation(uint64_t batchId,
                                                       const crypto::CurvePoint& batchVerificationInformation) {
    if (m_batchesVerificationInformation.contains(batchId)) {
        return;
    }

    m_batchesVerificationInformation[batchId] = batchVerificationInformation;

    if (m_batchesVerificationInformation.size() > m_maxBatchesHistorySize) {
        m_batchesVerificationInformation.erase(m_batchesVerificationInformation.begin());
    }
}

bool ProofOfExecution::verifyProof(const ExecutorKey& executorKey,
                                   const ExecutorInfo& executorInfo,
                                   const blockchain::Proofs& proof,
                                   uint64_t batchId,
                                   const crypto::CurvePoint& verificationInformation) {
    Hash512 dHash;
    crypto::Sha3_512_Builder dHasher;
    dHasher.update({proof.m_tProof.m_F.toBytes(), proof.m_batchProof.m_T.toBytes(), executorKey});
    dHasher.final(dHash);
    crypto::Scalar d(dHash.array());

    const auto base = crypto::CurvePoint::BasePoint();

    if (proof.m_tProof.m_F != proof.m_tProof.m_k * base + d * proof.m_batchProof.m_T) {
        return false;
    }

    crypto::CurvePoint previousT;
    crypto::Scalar previousR;
    uint64_t verifyStartBatchId = 0;

    if (executorInfo.m_initialBatch == proof.m_initialBatch) {
        previousT = executorInfo.m_batchProof.m_T;
        previousR = executorInfo.m_batchProof.m_r;
        verifyStartBatchId = executorInfo.m_nextBatchToApprove;
    } else if (executorInfo.m_nextBatchToApprove <= proof.m_initialBatch + 1) {
        // Actually the condition should be "<" here.
        // But due to possible executor's restart the situation with "<=" can occur
        verifyStartBatchId = proof.m_initialBatch;
    } else {
        return false;
    }

    crypto::CurvePoint left = proof.m_batchProof.m_T - previousT;
    crypto::CurvePoint right = (proof.m_batchProof.m_r - previousR) * crypto::CurvePoint::BasePoint();

    auto verificationInfoIt = m_batchesVerificationInformation.find(verifyStartBatchId);
    for (uint i = verifyStartBatchId; i < batchId; i++, verificationInfoIt++) {

        if (verificationInfoIt == m_batchesVerificationInformation.end()) {
            return false;
        }

        if (verificationInfoIt->first != i) {
            return false;
        }

        const auto& verificationInfo = verificationInfoIt->second;
        crypto::Sha3_512_Builder hasher_h2;
        Hash512 cHash;
        hasher_h2.update({base.toBytes(), verificationInfo.toBytes(), executorKey});
        hasher_h2.final(cHash);
        crypto::Scalar c(cHash.array());
        right += c * verificationInfo;
    }

    {
        crypto::Sha3_512_Builder hasher_h2;
        Hash512 cHash;
        hasher_h2.update({base.toBytes(), verificationInformation.toBytes(), executorKey});
        hasher_h2.final(cHash);
        crypto::Scalar c(cHash.array());
        right += c * verificationInformation;
    }

    if (left != right) {
        return false;
    }

    return true;

}

std::pair<crypto::Scalar, crypto::CurvePoint> ProofOfExecution::verificationInfo(uint64_t digest) {
    Hash512 digest_hash;
    sirius::crypto::Sha3_512_Builder hasher_h;

    hasher_h.update(utils::RawBuffer{reinterpret_cast<const uint8_t*>(&digest), sizeof(digest)});
    hasher_h.final(digest_hash);

    crypto::Scalar alpha = digest_hash.array();
    return {alpha, alpha * sirius::crypto::CurvePoint::BasePoint()};
}

} // namespace sirius::contract