/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <common/PlaceHolder.h>

namespace sirius::contract {

const char* supercontract_category_impl::name() const noexcept {
    return "supercontract";
}

std::string supercontract_category_impl::message(int ev) const {
    switch (ev) {
    case static_cast<unsigned int>(supercontract_error::virtual_machine_unavailable):
        return "Virtual Machine handler is absent";
    case static_cast<unsigned int>(supercontract_error::storage_unavailable):
        return "Storage handler is absent";
    case static_cast<unsigned int>(supercontract_error::internet_incorrect_query):
        return "Incorrect internet query";
    case static_cast<unsigned int>(supercontract_error::storage_incorrect_query):
        return "Incorrect storage query";
    default:
        return "Other error";
    }
}

bool supercontract_category_impl::equivalent(
    const std::error_code& code,
    int condition) const noexcept {
    switch (condition) {
    case static_cast<unsigned int>(supercontract_error::virtual_machine_unavailable):
        return code == std::errc::io_error || code == std::errc::resource_unavailable_try_again || code == std::errc::not_supported;
    case static_cast<unsigned int>(supercontract_error::storage_unavailable):
        return code == std::errc::io_error || code == std::errc::resource_unavailable_try_again || code == std::errc::not_supported;
    case static_cast<unsigned int>(supercontract_error::internet_unavailable):
        return code == std::errc::io_error || code == std::errc::resource_unavailable_try_again || code == std::errc::not_supported;
    case static_cast<unsigned int>(supercontract_error::internet_incorrect_query):
        return code == std::errc::io_error || code == std::errc::bad_message || code == std::errc::function_not_supported || code == std::errc::invalid_argument || code == std::errc::not_supported;
    case static_cast<unsigned int>(supercontract_error::storage_incorrect_query):
        return code == std::errc::io_error || code == std::errc::bad_message || code == std::errc::function_not_supported || code == std::errc::invalid_argument || code == std::errc::not_supported;
    default:
        return false;
    }
}

} // namespace sirius::contract

namespace std {
const error_category& api_category() {
    static sirius::contract::supercontract_category_impl instance;
    return instance;
}

error_condition make_error_condition(sirius::contract::supercontract_error e) {
    return error_condition(
        static_cast<int>(e),
        api_category());
}

error_code make_error_code(sirius::contract::supercontract_error e) {
    return error_code(
        static_cast<int>(e),
        api_category());
}
} // namespace std