#include "ProofOfExecution.h"
#include "utils/Random.h"
extern "C"
{
#include <external/ref10/sc.h>
}

namespace sirius::contract
{
    ProofOfExecution::ProofOfExecution(GlobalEnvironment &environment, std::array<uint8_t, 32> &publicKey, int seed) : m_publicKey(publicKey),
                                                                                                                       m_x(sirius::crypto::Scalar()),
                                                                                                                       m_xPrevious(sirius::crypto::Scalar()),
                                                                                                                       m_environment(environment)
    {
        srand(seed);
    }

    void ProofOfExecution::addToProof(u_int64_t digest)
    {
        ASSERT(isSingleThread(), m_environment.logger())

        sirius::crypto::CurvePoint beta;
        Hash512 digest_hash;
        Hash512 temp;
        sirius::crypto::Sha3_512_Builder hasher_h;

        std::memcpy(temp.data(), &digest, sizeof digest);
        hasher_h.update(temp);
        hasher_h.final(digest_hash);

        sirius::crypto::Scalar digest_scalar(digest_hash.array());
        sirius::crypto::CurvePoint Y = digest_scalar * beta;

        sirius::crypto::Sha3_512_Builder hasher_h2;
        Hash512 c;
        hasher_h2.update({beta.tobytes(), Y.tobytes(), this->m_publicKey});
        hasher_h2.final(c);
        sirius::crypto::Scalar c_scalar(c.array());

        this->m_xPrevious = this->m_x;
        this->m_x += c_scalar * digest_scalar;
    }

    void ProofOfExecution::popFromProof()
    {
        ASSERT(isSingleThread(), m_environment.logger())

        this->m_x = this->m_xPrevious;
    }

    Proofs ProofOfExecution::buildProof()
    {
        ASSERT(isSingleThread(), m_environment.logger())

        Hash512 v_r = sirius::utils::generateRandomByteValue<Hash512>();
        sirius::crypto::Scalar v(v_r.array());
        sirius::crypto::CurvePoint beta;
        sirius::crypto::CurvePoint T = v * beta;
        sirius::crypto::Scalar r = v - this->m_x;
        // this->m_b = std::make_tuple(T, r);
        BatchProof b{T, r};

        Hash512 w_r = sirius::utils::generateRandomByteValue<Hash512>();
        sirius::crypto::Scalar w(w_r.array());
        sirius::crypto::CurvePoint F = w * beta;

        Hash512 d_hash;
        sirius::crypto::Sha3_512_Builder hasher_h;
        hasher_h.update({F.tobytes(), T.tobytes(), this->m_publicKey});
        hasher_h.final(d_hash);

        sirius::crypto::Scalar d(d_hash.array());
        sirius::crypto::Scalar k = w - d * v;

        // this->m_q = std::make_tuple(F, k);
        TProof q{F, k};
        return Proofs{q, b};
    }

    void ProofOfExecution::reset()
    {
        ASSERT(isSingleThread(), m_environment.logger())

        this->m_x = sirius::crypto::Scalar();
        this->m_xPrevious = sirius::crypto::Scalar();
    }
}