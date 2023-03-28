/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <system_error>

namespace sirius::contract::internet {

enum class InternetError {
    internet_unavailable = 1,
    connections_limit_reached,
    incorrect_query,
    invalid_url_error,
    invalid_resource_error,
    connection_closed_error,
    resolve_error,
    connection_error,
    write_error,
    read_error,
    handshake_error,
    invalid_ocsp_error
};

class InternetErrorCategory
        : public std::error_category {
public:
    const char* name() const noexcept override;

    std::string message(int ev) const override;

    bool equivalent(int code, const std::error_condition& condition) const noexcept override;
};

const std::error_category& internetErrorCategory();

std::error_code make_error_code(InternetError e);

}

namespace std {

template<>
struct is_error_code_enum<sirius::contract::internet::InternetError>
        : public true_type {
};

}
