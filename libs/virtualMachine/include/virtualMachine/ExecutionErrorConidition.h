/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <system_error>

namespace sirius::contract::vm {

enum class ExecutionError {
    virtual_machine_unavailable = 1,
    storage_unavailable,
    internet_unavailable,
    storage_incorrect_query,
    internet_incorrect_query,
    blockchain_incorrect_query,
    blockchain_unavailable
};

class ExecutionErrorCategory
        : public std::error_category {
public:
    const char* name() const noexcept override;

    std::string message(int ev) const override;

    bool equivalent(const std::error_code& code, int condition) const noexcept override;
};

const std::error_category& executionErrorCategory();

std::error_condition make_error_condition(ExecutionError e);

}

namespace std {

template<>
struct is_error_condition_enum<sirius::contract::vm::ExecutionError>
        : public true_type {
};

}
