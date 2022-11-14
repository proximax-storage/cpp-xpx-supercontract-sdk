extern "C" {
#include <external/ref10/ge.h>
}

#include "Scalar.h"
#include "crypto/KeyPair.h"
#include <cereal/types/array.hpp>

namespace sirius::crypto {

class CurvePoint {

    ge_p3 m_ge_p3;

public:
    CurvePoint();

    static CurvePoint BasePoint();

    CurvePoint operator+(CurvePoint& a) const;

    CurvePoint operator-(CurvePoint& a) const;

    CurvePoint operator-() const;

    CurvePoint operator*(Scalar& a) const;

    friend CurvePoint operator*(Scalar& a, CurvePoint& b);

    CurvePoint& operator+=(CurvePoint& a);

    CurvePoint& operator-=(CurvePoint& a);

    CurvePoint& operator*=(Scalar& a);

    bool operator==(const CurvePoint& a) const;

    bool operator!=(const CurvePoint& a) const;

    std::array<uint8_t, 32> toBytes() const;

    void fromBytes(const std::array<uint8_t, 32>& buffer);

    template<class Archive>
    void save(Archive& archive) const {
        archive(toBytes());
    }

    template<class Archive>
    void load(Archive& archive) {
        std::array<uint8_t, 32> buffer{};
        archive(buffer);
        fromBytes(buffer);
    }
};

}