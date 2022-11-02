/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <system_error>

namespace sirius::contract {

enum class supercontract_error {
    virtual_machine_unavailable,
    storage_unavailable,
    internet_unavailable,
    storage_incorrect_query,
    internet_incorrect_query
};

class supercontract_category_impl
        : public std::error_category {
public:
    const char* name() const noexcept override;

    std::string message(int ev) const override;

    bool equivalent(int code,
                    const std::error_condition& condition) const noexcept override;
};

const std::error_category& supercontract_category();

std::error_code make_error_code(sirius::contract::supercontract_error e);

}

namespace std {

template<>
struct is_error_code_enum<sirius::contract::supercontract_error>
        : public true_type {
};

}
