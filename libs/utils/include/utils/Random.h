/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <random>

namespace sirius::utils {

    template <class T>
    T generateRandomByteValue() {
        std::random_device rd;
        std::seed_seq sd{ rd(), rd(), rd(), rd() };
        std::mt19937 mt(sd);
        T value;
        std::generate_n(reinterpret_cast<uint8_t *>(&value), sizeof(value), mt);
        return value;
    }

    template<class T, class Seed>
    T generateRandomByteValue(const Seed& seed) {
        std::random_device rd;
        std::seed_seq sd(seed.begin(), seed.end());
        std::mt19937 mt(sd);
        T value;
        std::generate_n(reinterpret_cast<uint8_t*>(&value), sizeof(value), mt);
        return value;
    }

}