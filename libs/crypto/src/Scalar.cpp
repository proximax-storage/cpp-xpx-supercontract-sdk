#include "crypto/Scalar.h"
extern "C"
{
#include <external/ref10/sc.h>
}

namespace sirius
{
    namespace crypto
    {
        Scalar Scalar::operator+(const Scalar &a) const
        {
            Scalar temp;

            // sc_muladd(temp.data(), );

            return temp;
        }
    }
}