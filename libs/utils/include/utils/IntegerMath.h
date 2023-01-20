/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include <cstdint>
#include <limits>
#include <type_traits>

namespace sirius { namespace utils {

	/// Adds \a delta to \a value if and only if there is no overflow.
	template<typename T>
	bool CheckedAdd(T& value, T delta) {
		if (value > std::numeric_limits<T>::max() - delta)
			return false;

		value += delta;
		return true;
	}

	/// Gets the number of bits in the specified type.
	template<typename T, typename X = std::enable_if_t<std::is_integral_v<T>>>
	constexpr T GetNumBits() {
		return static_cast<T>(8u * sizeof(T));
	}

	/// Calculates log2(\a value).
	template<typename T, typename X = std::enable_if_t<std::is_unsigned_v<T>>>
	constexpr T Log2(T value) {
#ifdef _MSC_VER
		unsigned long result;
		if (!_BitScanReverse(&result, value))
			return std::numeric_limits<T>::max();

		return static_cast<T>(result);
#else
		if (!value)
			return std::numeric_limits<T>::max();

		return static_cast<T>(63 - __builtin_clzll(value));
#endif
	}

	/// Calculates log2(\a value^(2^\a n)).
	uint64_t Log2TimesPowerOfTwo(uint64_t value, uint64_t n);

	/// Calculates 2^(\a value).
	template<typename T, typename X = std::enable_if_t<std::is_unsigned_v<T>>>
	constexpr T Pow2(T value) {
		return value >= GetNumBits<T>() ? 0 : static_cast<T>(static_cast<T>(1) << value);
	}

	/// Divides \a value by \a divisor and returns the remainder.
	template<typename T, typename X = std::enable_if_t<std::is_unsigned_v<T>>>
	constexpr T DivideAndGetRemainder(T& value, T divisor) {
		auto remainder = static_cast<T>(value % divisor);
		value /= divisor;
		return remainder;
	}

	/// Returns \c true if \a rhs is equal to \a lhs multipled by a power of \a base.
	template<typename T, typename X = std::enable_if_t<std::is_unsigned_v<T>>>
	constexpr bool IsPowerMultiple(T lhs, T rhs, T base) {
		if (lhs > rhs || 0 != rhs % lhs)
			return false;

		T quotient = rhs / lhs;
		while (quotient > 1) {
			if (0 != DivideAndGetRemainder(quotient, base))
				return false;
		}

		return true;
	}

    template<typename T, typename X = std::enable_if_t<std::is_unsigned_v<T>>>
    constexpr T DivideCeil(T numerator, T denominator) {
        if (numerator == static_cast<T>(0)) {
            return static_cast<T>(0);
        }
        return (numerator - static_cast<T>(1)) / denominator + static_cast<T>(1);
    }
}}
