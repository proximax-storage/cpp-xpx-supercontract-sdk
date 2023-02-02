/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <vector>
#include <cstdint>

#include <cereal/archives/portable_binary.hpp>

namespace sirius::utils {

template<class T>
    std::string plainBytes(const T* ptr, size_t size) {
    return std::string(reinterpret_cast<const char*>(ptr), size);
}

template<class T>
std::string plainBytes(const T& value) {
    return plainBytes(&value, sizeof(value));
}

template<class T>
std::string serialize(const T& value) {
    std::ostringstream os(std::ios::binary);
    cereal::PortableBinaryOutputArchive archive(os);
    archive(value);
    return os.str();
}

template<class T>
T deserialize(const std::string& value) {
    std::istringstream is(value, std::ios::binary);
    cereal::PortableBinaryInputArchive iarchive(is);
    T info;
    iarchive(info);
    return info;
}

void copyToVector(std::vector<uint8_t>& data, const uint8_t* p, size_t bytes);

}