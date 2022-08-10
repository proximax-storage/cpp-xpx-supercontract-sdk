/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <random>

namespace sirius::utils {

    template <class T>
    T generateRandomByteValue() {
        T value;
        std::random_device rd;
        std::seed_seq sd{ rd(), rd(), rd(), rd() };
        std::mt19937 mt(sd);
        std::generate_n(value.data(), sizeof(T), mt);
        return value;
    }

}