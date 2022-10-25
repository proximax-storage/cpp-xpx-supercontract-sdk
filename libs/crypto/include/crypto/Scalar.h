#include "utils/ByteArray.h"

namespace sirius { namespace crypto {

constexpr size_t Scalar_Size = 32;

struct Scalar_Tag {
    static constexpr auto Byte_Size = 32;
};

class Scalar : public sirius::utils::ByteArray<Scalar_Size, Scalar_Tag> {

private:
    static constexpr std::array<uint8_t, Scalar_Size> L_MINUS_ONE = {236, 211, 245, 92, 26, 99, 18, 88, 214, 156, 247, 162, 222, 249, 222, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16};

public:
    Scalar();
    ~Scalar();
    Scalar(const std::array<uint8_t, Scalar_Size * 2>& arr);
    Scalar(const std::array<uint8_t, Scalar_Size>& arr);
    static Scalar getLMinusOne();
    Scalar operator+(const Scalar& a) const;
    Scalar operator*(const Scalar& a) const;
    Scalar operator-(const Scalar& a) const;
    Scalar operator-() const;
    Scalar& operator+=(const Scalar& a);
    Scalar& operator*=(const Scalar& a);
    Scalar& operator-=(const Scalar& a);
    Scalar addProduct(const Scalar& r, const Scalar& h) const;
};

}} // namespace sirius::crypto