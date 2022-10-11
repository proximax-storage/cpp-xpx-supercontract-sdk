extern "C"
{
#include <external/ref10/ge.h>
}
#include "Scalar.h"

namespace sirius { namespace crypto {
        class CurvePoint
        {
            ge_p3 m_ge_p3;

        public:
            CurvePoint();
            static CurvePoint BasePoint();
            CurvePoint operator+(CurvePoint &a) const;
            CurvePoint operator-(CurvePoint &a) const;
            CurvePoint operator*(Scalar &a) const;
            friend CurvePoint operator*(Scalar &a, CurvePoint&b);
            CurvePoint &operator+=(CurvePoint &a);
            CurvePoint &operator-=(CurvePoint &a);
            CurvePoint &operator*=(Scalar &a);
            bool operator==(const CurvePoint &a) const;
        };
    }
}