#include "crypto/CurvePoint.h"

namespace sirius
{
    namespace crypto
    {
        CurvePoint::CurvePoint() : m_ge_p3(ge_p3())
        {
        }

        CurvePoint CurvePoint::BasePoint()
        {
            CurvePoint temp;
            ge_p3 a;
            ge_p3_0(&a);
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

        CurvePoint CurvePoint::operator*(Scalar &a)
        {
            CurvePoint ret;
            ge_scalarmult_base(&this->m_ge_p3, a.data());
            ret.m_ge_p3 = this->m_ge_p3;
            return ret;
        }

        void CurvePoint::operator+=(CurvePoint &a)
        {
            *this = *this + a;
        }

        void CurvePoint::operator-=(CurvePoint &a)
        {
            *this = *this - a;
        }

        void CurvePoint::operator*=(Scalar &a)
        {
            *this = *this * a;
        }

        bool CurvePoint::operator==(const CurvePoint &a) const
        {
            std::vector<uint8_t> this_bytes;
            this_bytes.resize(32);
            ge_p3_tobytes(this_bytes.data(), &this->m_ge_p3);
            std::vector<uint8_t> a_bytes;
            a_bytes.resize(32);
            ge_p3_tobytes(a_bytes.data(), &a.m_ge_p3);

            return this_bytes == a_bytes;
        }
    }
}