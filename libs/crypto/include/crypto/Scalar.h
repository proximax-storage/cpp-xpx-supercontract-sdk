#include "utils/ByteArray.h"

namespace sirius
{
    namespace crypto
    {
        constexpr size_t Scalar_Size = 32;
        struct Scalar_Tag
        {
            static constexpr auto Byte_Size = 32;
        };
        class Scalar : public sirius::utils::ByteArray<Scalar_Size, Scalar_Tag>
        {
        public:
            Scalar operator+(const Scalar &a) const;
            Scalar operator*(const Scalar &a) const;
            Scalar operator-(const Scalar &a) const;
            Scalar &operator+=(const Scalar &a);
            Scalar &operator*=(const Scalar &a);
            Scalar &operator-=(const Scalar &a);
            Scalar addProduct(const Scalar &r, const Scalar &h, const Scalar &a) const;
        };
    }
}