/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once
#include "utils/ByteArray.h"
#include "utils/RawBuffer.h"
#include <string>
#include <array>
#include <set>
#include <functional>
#include <boost/asio/ip/tcp.hpp>


namespace sirius {

	// region byte arrays (ex address)

	constexpr size_t Signature_Size = 64;
	constexpr size_t Key_Size = 32;
	constexpr size_t Hash512_Size = 64;
	constexpr size_t Hash256_Size = 32;
	constexpr size_t Hash160_Size = 20;

	struct Signature_tag {};
	using Signature = utils::ByteArray<Signature_Size, Signature_tag>;

    struct Key_tag { static constexpr auto Byte_Size = 32; };
	using Key = utils::ByteArray<Key_Size, Key_tag>;

	struct Hash512_tag { static constexpr auto Byte_Size = 64; };
	using Hash512 = utils::ByteArray<Hash512_Size, Hash512_tag>;

	struct Hash256_tag { static constexpr auto Byte_Size = 32; };
	using Hash256 = utils::ByteArray<Hash256_Size, Hash256_tag>;

	struct Hash160_tag {};
	using Hash160 = utils::ByteArray<Hash160_Size, Hash160_tag>;

	struct GenerationHash_tag { static constexpr auto Byte_Size = 32; };
	using GenerationHash = utils::ByteArray<Hash256_Size, GenerationHash_tag>;

    struct Timestamp_tag {};
    using Timestamp = utils::BaseValue<uint64_t, Timestamp_tag>;

    constexpr size_t Address_Decoded_Size = 25;
    constexpr size_t Address_Encoded_Size = 40;

    struct Address_tag {};
    using Address = utils::ByteArray<Address_Decoded_Size, Address_tag>;

    // endregion

	template<typename T, size_t N>
	constexpr size_t CountOf(T const (&)[N]) noexcept {
		return N;
	}

    namespace contract {

#define DECL_KEY(KeyName) \
        struct KeyName : public Key \
        { \
            KeyName() = default; \
            KeyName( const Key& key ) : Key(key) {} \
            KeyName( const std::array<uint8_t,32>& array ) : Key(array) {} \
            KeyName( const std::string& s ) : KeyName(*reinterpret_cast<const KeyName*>(s.data())) {} \
            std::string toString() const {     \
                return std::string(reinterpret_cast<const char*>(data()), Key_Size);\
            } \
            int operator[]( std::array<uint8_t,32>::size_type pos ) const { return (int) static_cast<const Key&>(*this)[pos]; } \
            \
            template<class Archive> \
            void serialize(Archive &arch) \
            { \
                arch( m_array ); \
            } \
        };

#define DECL_HASH(HashName) \
        struct HashName : public Hash256 \
        { \
            HashName() = default; \
            HashName( const Hash256& key ) : Hash256(key) {} \
            HashName( const std::array<uint8_t,32>& array ) : Hash256(array) {} \
            HashName( const std::string& s ) : HashName(*reinterpret_cast<const HashName*>(s.data())) {} \
            std::string toString() const {     \
                return std::string(reinterpret_cast<const char*>(data()), Hash256_Size);\
            } \
            int operator[]( std::array<uint8_t,32>::size_type pos ) const { return (int) static_cast<const Hash256&>(*this)[pos]; } \
            \
            template<class Archive> \
            void serialize(Archive &arch) \
            { \
                arch( m_array ); \
            } \
        };
    }
}

