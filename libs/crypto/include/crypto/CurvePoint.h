extern "C"
{
#include <external/ref10/ge.h>
}
#include "Scalar.h"

namespace sirius
{
    namespace crypto
    {
        class CurvePoint
        {
            ge_p3 m_ge_p3;

        public:
            CurvePoint();
            static CurvePoint BasePoint();
            CurvePoint operator+(CurvePoint &a) const;
            CurvePoint operator-(CurvePoint &a) const;
            CurvePoint operator*(Scalar &a);
            void operator+=(CurvePoint &a);
            void operator-=(CurvePoint &a);
            void operator*=(Scalar &a);
            bool operator==(const CurvePoint &a) const;
        };
    }
}