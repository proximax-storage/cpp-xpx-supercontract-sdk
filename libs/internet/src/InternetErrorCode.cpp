/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <internet/InternetErrorCode.h>

namespace sirius::contract::internet {

const char* InternetErrorCategory::name() const noexcept {
    return "internet";
}

std::string InternetErrorCategory::message(int ev) const {
    switch (ev) {
        case static_cast<int>(InternetError::internet_unavailable):
            return "Internet is unavailable";
        case static_cast<int>(InternetError::incorrect_query):
            return "Incorrect query";
        case static_cast<int>(InternetError::connection_closed_error):
            return "Connection is closed";
        case static_cast<int>(InternetError::resolve_error):
            return "Fail to resolve url";
        case static_cast<int>(InternetError::connection_error):
            return "Fail to establish connection";
        case static_cast<int>(InternetError::write_error):
            return "Fail to write stream";
        case static_cast<int>(InternetError::read_error):
            return "Fail to read stream";
        case static_cast<int>(InternetError::handshake_error):
            return "Fail to handshake";
        case static_cast<int>(InternetError::invalid_ocsp_error):
            return "OCSP is invalid";
        default:
            return "Other error";
    }
}

bool InternetErrorCategory::equivalent(int code, const std::error_condition& condition) const noexcept {
    return false;
}

const std::error_category& internetErrorCategory() {
    static InternetErrorCategory instance;
    return instance;
}

std::error_code make_error_code(InternetError e) {
    return {static_cast<int>(e), internetErrorCategory()};
}

} // namespace sirius::contract::storage
