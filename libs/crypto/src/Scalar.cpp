#include "crypto/Scalar.h"
extern "C"
{
#include <external/ref10/sc.h>
}

namespace sirius
{
    namespace crypto
    {
        Scalar Scalar::getI()
        {
            Scalar ret;
            ret.m_array = Scalar::I_MINUS_ONE;
            return ret;
        }

        Scalar Scalar::operator+(const Scalar &a) const
        {
            Scalar temp;
            Scalar one;
            one[0] = 1;
            sc_muladd(temp.data(), this->m_array.data(), one.m_array.data(), a.m_array.data());

            return temp;
        }

        Scalar Scalar::operator*(const Scalar &a) const
        {
            Scalar temp;
            Scalar zero;
            sc_muladd(temp.data(), this->m_array.data(), a.m_array.data(), zero.m_array.data());

            return temp;
        }

        Scalar Scalar::operator-(const Scalar &a) const
        {
            Scalar temp;
            Scalar iMinusOne = Scalar::getI();
            sc_muladd(temp.data(), a.m_array.data(), iMinusOne.m_array.data(), this->m_array.data());

            return temp;
        }

        void Scalar::operator+=(const Scalar &a)
        {
            *this = *this + a;
        }

        void Scalar::operator-=(const Scalar &a)
        {
            *this = *this - a;
        }

        void Scalar::operator*=(const Scalar &a)
        {
            *this = *this * a;
        }

        Scalar Scalar::addProduct(const Scalar &r, const Scalar &h) const
        {
            Scalar temp;
            sc_muladd(temp.data(), r.m_array.data(), h.m_array.data(), this->m_array.data());
            return temp;
        }
    }
}