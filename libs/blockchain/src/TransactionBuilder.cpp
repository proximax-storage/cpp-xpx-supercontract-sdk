/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <blockchain/TransactionBuilder.h>
#include <climits>
#include <supercontract/Identifiers.h>

namespace sirius::contract::blockchain {

namespace {

constexpr VersionType makeVersion(NetworkIdentifier networkIdentifier, VersionType version) noexcept {
    constexpr uint8_t NETWORK_IDENTIFIER_SHIFT = (sizeof(VersionType) - sizeof(NetworkIdentifier)) * CHAR_BIT;
    return static_cast<VersionType>(networkIdentifier) << NETWORK_IDENTIFIER_SHIFT | version;
}

template<class T>
void pushBytes(std::vector<uint8_t>& buffer, T* data, size_t size) {
    auto* begin = reinterpret_cast<const uint8_t*>(data);
    buffer.insert(buffer.end(), begin, begin + size);
}

}

std::vector<uint8_t> buildEmbeddedTransaction(NetworkIdentifier networkIdentifier,
                                              const ContractKey& contractKey,
                                              const EmbeddedTransaction& transaction) {
    auto version = makeVersion(networkIdentifier, transaction.m_version);

    uint32_t size = 0;
    size = sizeof(size) + sizeof(contractKey) + sizeof(version) + sizeof(transaction.m_entityType) +
            transaction.m_payload.size();

    std::vector<uint8_t> payload;
    payload.reserve(size);
    pushBytes(payload, &size, sizeof(size));
    pushBytes(payload, contractKey.data(), contractKey.size());
    pushBytes(payload, &version, sizeof(version));
    pushBytes(payload, &transaction.m_entityType, sizeof(transaction.m_entityType));
    pushBytes(payload, transaction.m_payload.data(), transaction.m_payload.size());

    return payload;
}

}