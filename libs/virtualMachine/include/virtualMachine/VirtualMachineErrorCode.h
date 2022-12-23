/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <system_error>

namespace sirius::contract::vm {

enum class VirtualMachineError {
    vm_unavailable = 1
};

class VirtualMachineErrorCategory
        : public std::error_category {
public:
    const char* name() const noexcept override;

    std::string message(int ev) const override;

    bool equivalent(int code, const std::error_condition& condition) const noexcept override;
};

const std::error_category& virtualMachineErrorCategory();

std::error_code make_error_code(VirtualMachineError e);

}

namespace std {

template<>
struct is_error_code_enum<sirius::contract::vm::VirtualMachineError>
        : public true_type {
};

}
