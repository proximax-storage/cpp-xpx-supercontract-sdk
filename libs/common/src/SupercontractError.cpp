/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <common/SupercontractError.h>

namespace sirius::contract {

const char* supercontract_category_impl::name() const noexcept {
    return "supercontract";
}

std::string supercontract_category_impl::message(int ev) const {
    switch (ev) {
        case static_cast<unsigned int>(supercontract_error::virtual_machine_unavailable):
            return "Virtual Machine is unavailable";
        case static_cast<unsigned int>(supercontract_error::storage_unavailable):
            return "Storage is unavailable";
        case static_cast<unsigned int>(supercontract_error::internet_unavailable):
            return "Internet is unavailable";
        case static_cast<unsigned int>(supercontract_error::internet_incorrect_query):
            return "Incorrect internet query";
        case static_cast<unsigned int>(supercontract_error::storage_incorrect_query):
            return "Incorrect storage query";
        default:
            return "Other error";
    }
}

bool supercontract_category_impl::equivalent(int code,
                                             const std::error_condition& condition) const noexcept {
    return false;
}

const std::error_category& supercontract_category() {
    static sirius::contract::supercontract_category_impl instance;
    return instance;
}

std::error_code make_error_code(sirius::contract::supercontract_error e) {
    return {static_cast<int>(e), supercontract_category()};
}

} // namespace sirius::contract