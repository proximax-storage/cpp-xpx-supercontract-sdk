/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <system_error>

namespace sirius::contract::blockchain {

enum class BlockchainError {
    blockchain_unavailable = 1,
    incorrect_query,
    };

class BlockchainErrorCategory
        : public std::error_category {
public:
    const char* name() const noexcept override;

    std::string message(int ev) const override;

    bool equivalent(int code, const std::error_condition& condition) const noexcept override;
};

const std::error_category& blockchainErrorCategory();

std::error_code make_error_code(BlockchainError e);

}

namespace std {

template<>
struct is_error_code_enum<sirius::contract::blockchain::BlockchainError>
        : public true_type {
};

}
