#include "ProofOfExecution.h"
#include "utils/Random.h"

namespace sirius::contract
{
    ProofOfExecution::ProofOfExecution(GlobalEnvironment &environment, std::array<uint8_t, 32> &publicKey, int seed) : m_publicKey(Hash256(publicKey)),
                                                                                                                       m_b(std::make_tuple(sirius::crypto::CurvePoint(), sirius::crypto::Scalar())),
                                                                                                                       m_q(std::make_tuple(sirius::crypto::CurvePoint(), sirius::crypto::Scalar())),
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
        Hash256 digest_hash;
        sirius::crypto::Scalar temp;
        sirius::crypto::Sha3_256_Builder hasher_h;

        std::memcpy(temp.data(), &digest, sizeof digest);
        hasher_h.update(temp);
        hasher_h.final(digest_hash);

        sirius::crypto::Scalar digest_scalar(digest_hash.array());
        sirius::crypto::CurvePoint Y = digest_scalar * beta;

        sirius::crypto::Sha3_256_Builder hasher_h2;
        Hash256 c;
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

    void ProofOfExecution::buildProof()
    {
        ASSERT(isSingleThread(), m_environment.logger())

        sirius::crypto::Scalar v = sirius::utils::generateRandomByteValue<sirius::crypto::Scalar>();
        sirius::crypto::CurvePoint beta;
        sirius::crypto::CurvePoint T = v * beta;
        sirius::crypto::Scalar r = v - this->m_x;
        this->m_b = std::make_tuple(T, r);

        sirius::crypto::Scalar w = sirius::utils::generateRandomByteValue<sirius::crypto::Scalar>();
        sirius::crypto::CurvePoint F = w * beta;

        Hash256 d_hash;
        sirius::crypto::Sha3_256_Builder hasher_h;
        hasher_h.update({F.tobytes(), T.tobytes(), this->m_publicKey});
        hasher_h.final(d_hash);
        sirius::crypto::Scalar d(d_hash.array());
        sirius::crypto::Scalar k = w - d * v;

        this->m_q = std::make_tuple(F, k);
    }

    void ProofOfExecution::reset()
    {
        ASSERT(isSingleThread(), m_environment.logger())

        this->m_x = sirius::crypto::Scalar();
        this->m_xPrevious = sirius::crypto::Scalar();
        this->m_b = std::make_tuple(sirius::crypto::CurvePoint(), sirius::crypto::Scalar());
        this->m_q = std::make_tuple(sirius::crypto::CurvePoint(), sirius::crypto::Scalar());
    }
}