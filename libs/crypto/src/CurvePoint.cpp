#include "crypto/CurvePoint.h"

namespace sirius { namespace crypto {
        CurvePoint::CurvePoint()
        {
            ge_p3 a;
            ge_p3_0(&a);
            this->m_ge_p3 = a;
        }

        CurvePoint CurvePoint::BasePoint()
        {
            CurvePoint temp;
            ge_p3 a;
            Scalar one;
            one[0] = 1;
            ge_scalarmult_base(&a, one.data());
            temp.m_ge_p3 = a;
            return temp;
        }

        CurvePoint CurvePoint::operator+(CurvePoint &a) const
        {
            CurvePoint temp;
            ge_cached cache;
            ge_p3_to_cached(&cache, &a.m_ge_p3);

            ge_p1p1 ans;
            ge_add(&ans, &this->m_ge_p3, &cache);
            ge_p1p1_to_p3(&temp.m_ge_p3, &ans);
            return temp;
        }

        CurvePoint CurvePoint::operator-(CurvePoint &a) const
        {
            CurvePoint temp;
            ge_cached cache;
            ge_p3_to_cached(&cache, &a.m_ge_p3);

            ge_p1p1 ans;
            ge_sub(&ans, &this->m_ge_p3, &cache);
            ge_p1p1_to_p3(&temp.m_ge_p3, &ans);
            return temp;
        }

        CurvePoint CurvePoint::operator*(Scalar &a) const
        {
            CurvePoint ret;
            Scalar zero;
            ge_p2 ans;
            ge_double_scalarmult_vartime(&ans, a.data(), &this->m_ge_p3, zero.data());
            Scalar temp;
            ge_tobytes(temp.data(), &ans);

            ge_frombytes_negate_vartime(&ret.m_ge_p3, temp.data());
            auto offset = ret;
            ret -= offset;
            ret -= offset;
            return ret;
        }

        CurvePoint operator*(Scalar &a, CurvePoint &b)
        {
            CurvePoint ret;
            Scalar zero;
            ge_p2 ans;
            ge_double_scalarmult_vartime(&ans, a.data(), &b.m_ge_p3, zero.data());
            Scalar temp;
            ge_tobytes(temp.data(), &ans);

            ge_frombytes_negate_vartime(&ret.m_ge_p3, temp.data());
            auto offset = ret;
            ret -= offset;
            ret -= offset;
            return ret;
        }

        CurvePoint &CurvePoint::operator+=(CurvePoint &a)
        {
            *this = *this + a;
            return *this;
        }

        CurvePoint &CurvePoint::operator-=(CurvePoint &a)
        {
            *this = *this - a;
            return *this;
        }

        CurvePoint &CurvePoint::operator*=(Scalar &a)
        {
            *this = *this * a;
            return *this;
        }

        bool CurvePoint::operator==(const CurvePoint &a) const
        {
            Scalar this_bytes;
            ge_p3_tobytes(this_bytes.data(), &this->m_ge_p3);
            Scalar a_bytes;
            ge_p3_tobytes(a_bytes.data(), &a.m_ge_p3);

            return this_bytes == a_bytes;
        }

        bool CurvePoint::operator!=(const CurvePoint &a) const
        {
            Scalar this_bytes;
            ge_p3_tobytes(this_bytes.data(), &this->m_ge_p3);
            Scalar a_bytes;
            ge_p3_tobytes(a_bytes.data(), &a.m_ge_p3);

            return this_bytes != a_bytes;
        }

        Scalar CurvePoint::tobytes() const
        {
            Scalar ret;
            ge_p3_tobytes(ret.data(), &this->m_ge_p3);
            return ret;
        }
    }
}