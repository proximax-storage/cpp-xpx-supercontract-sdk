/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <blockchain/TransactionBuilder.h>
#include <climits>
#include <common/Identifiers.h>
#include <crypto/Hashes.h>

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

std::pair<Hash256, SerializedAggregatedTransaction>
buildAggregatedTransaction(NetworkIdentifier networkIdentifier, const ContractKey& contractKey,
                           const AggregatedTransaction& transaction) {
    SerializedAggregatedTransaction serializedAggregatedTransaction;
    serializedAggregatedTransaction.m_maxFee = transaction.m_maxFee;

    for (const auto& embeddedTransaction: transaction.m_transactions) {
        serializedAggregatedTransaction.m_transactions.emplace_back(
                buildEmbeddedTransaction(networkIdentifier, contractKey, embeddedTransaction));
    }

    crypto::Sha3_256_Builder hasher;
    hasher.update(utils::RawBuffer{
        reinterpret_cast<const uint8_t*>(&serializedAggregatedTransaction.m_maxFee),
        sizeof(serializedAggregatedTransaction.m_maxFee)});
    for (const auto& tx: serializedAggregatedTransaction.m_transactions) {
        hasher.update(utils::RawBuffer{tx.data(), tx.size()});
    }
    Hash256 releasedTransactionsHash;
    hasher.final(releasedTransactionsHash);
    return {releasedTransactionsHash, serializedAggregatedTransaction};
}

}