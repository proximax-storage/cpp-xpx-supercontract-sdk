#include "ProofOfExecution.h"
#include "crypto/PrivateKey.h"
#include "utils/Random.h"
extern "C" {
#include <external/ref10/sc.h>
}

namespace sirius::contract {
void HashPrivateKey(const sirius::crypto::PrivateKey& privateKey,
                    Hash512& hash) {
    sirius::crypto::Sha3_512({privateKey.data(), privateKey.size()}, hash);
}

sirius::crypto::Scalar hashPrivateKey(const crypto::KeyPair& key,
                                      const utils::RawBuffer& dataBuffer) {
    // Hash the private key to improve randomness.
    Hash512 privHash;
    HashPrivateKey(key.privateKey(), privHash);

    // r = H(privHash[256:512] || data)
    // "EdDSA avoids these issues by generating r = H(h_b, ..., h_2b?1, M), so
    // that
    //  different messages will lead to different, hard-to-predict values of r."
    Hash512 h;
    sirius::crypto::Sha3_512_Builder hasher_r;
    hasher_r.update({privHash.data() + Hash512_Size / 2, Hash512_Size / 2});
    hasher_r.update(dataBuffer);
    hasher_r.final(h);
    sirius::crypto::Scalar scalar(h.array());
    return scalar;
}

ProofOfExecution::ProofOfExecution(GlobalEnvironment& environment,
                                   const crypto::KeyPair& key)
    : m_keyPair(key), m_x(sirius::crypto::Scalar()),
      m_xPrevious(sirius::crypto::Scalar()), m_environment(environment) {}

sirius::crypto::CurvePoint ProofOfExecution::addToProof(uint64_t digest) {
    ASSERT(isSingleThread(), m_environment.logger())

    auto Beta = sirius::crypto::CurvePoint::BasePoint();
    Hash512 digest_hash;
    Hash512 temp;
    sirius::crypto::Sha3_512_Builder hasher_h;

    std::memcpy(temp.data(), &digest, sizeof digest);
    hasher_h.update(temp);
    hasher_h.final(digest_hash);

    sirius::crypto::Scalar alpha(digest_hash.array());
    auto Y = alpha * Beta;

    sirius::crypto::Sha3_512_Builder hasher_h2;
    Hash512 c;
    hasher_h2.update({Beta.tobytes(), Y.tobytes(), m_keyPair.publicKey()});
    hasher_h2.final(c);
    sirius::crypto::Scalar c_scalar(c.array());

    m_xPrevious = m_x;
    m_x += c_scalar * alpha;

    return Y;
}

void ProofOfExecution::popFromProof() {
    ASSERT(isSingleThread(), m_environment.logger())

    m_x = m_xPrevious;
}

Proofs ProofOfExecution::buildProof() {
    ASSERT(isSingleThread(), m_environment.logger())

    auto v = hashPrivateKey(m_keyPair, m_x);
    auto Beta = sirius::crypto::CurvePoint::BasePoint();
    auto T = v * Beta;
    auto r = v - m_x;
    // this->m_b = std::make_tuple(T, r);
    BatchProof b{T, r};

    // Hash512 w_r = sirius::utils::generateRandomByteValue<Hash512>();
    // sirius::crypto::Scalar w(w_r.array());
    auto w = hashPrivateKey(m_keyPair, v);
    auto F = w * Beta;

    Hash512 d_hash;
    sirius::crypto::Sha3_512_Builder hasher_h;
    hasher_h.update({F.tobytes(), T.tobytes(), m_keyPair.publicKey()});
    hasher_h.final(d_hash);

    sirius::crypto::Scalar d(d_hash.array());
    auto k = w - d * v;

    // this->m_q = std::make_tuple(F, k);
    TProof q{F, k};
    return Proofs{q, b};
}

void ProofOfExecution::reset() {
    ASSERT(isSingleThread(), m_environment.logger())

    m_x = sirius::crypto::Scalar();
    m_xPrevious = sirius::crypto::Scalar();
}
} // namespace sirius::contract