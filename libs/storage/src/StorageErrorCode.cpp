/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <storage/StorageErrorCode.h>

namespace sirius::contract::storage {

const char* StorageErrorCategory::name() const noexcept {
    return "storage";
}

std::string StorageErrorCategory::message(int ev) const {
    // TODO Define names
    return std::string();
}

bool StorageErrorCategory::equivalent(int code, const std::error_condition& condition) const noexcept {
    return false;
}

const std::error_category& storageErrorCategory() {
    static StorageErrorCategory instance;
    return instance;
}

std::error_code make_error_code(StorageError e) {
    return {static_cast<int>(e), storageErrorCategory()};
}

}
