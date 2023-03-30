/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <system_error>

namespace sirius::contract::executor {

enum class RPCExecutorError {
    read_error = 1,
    write_error,
    start_error,
    finish_error
};

class RPCExecutorErrorCategory
        : public std::error_category {
public:
    const char* name() const noexcept override;

    std::string message(int ev) const override;

    bool equivalent(int code, const std::error_condition& condition) const noexcept override;
};

const std::error_category& rpcExecutorErrorCategory();

std::error_code make_error_code(RPCExecutorError e);

}

namespace std {

template<>
struct is_error_code_enum<sirius::contract::executor::RPCExecutorError>
        : public true_type {
};

}
