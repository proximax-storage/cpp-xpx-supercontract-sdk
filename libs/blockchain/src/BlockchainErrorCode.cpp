/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <blockchain/BlockchainErrorCode.h>

namespace sirius::contract::blockchain {

const char* BlockchainErrorCategory::name() const noexcept {
    return "blockchain";
}

std::string BlockchainErrorCategory::message(int ev) const {
    switch (ev) {
    case static_cast<unsigned int>(BlockchainError::blockchain_unavailable):
        return "Blockchain is unavailable";
        case static_cast<unsigned int>(BlockchainError::incorrect_query):
        return "IncorrectQuery";
    default:
        return "Other error";
    }
}

bool BlockchainErrorCategory::equivalent(int code, const std::error_condition& condition) const noexcept {
    return false;
}

const std::error_category& blockchainErrorCategory() {
    static BlockchainErrorCategory instance;
    return instance;
}

std::error_code make_error_code(BlockchainError e) {
    return {static_cast<int>(e), blockchainErrorCategory()};
}

} // namespace sirius::contract::storage
