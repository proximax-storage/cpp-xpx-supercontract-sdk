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
    switch (ev) {
    case static_cast<unsigned int>(StorageError::storage_unavailable):
        return "Storage is unavailable";
    case static_cast<unsigned int>(StorageError::open_file_error):
        return "Fail to open the file";
    case static_cast<unsigned int>(StorageError::read_file_error):
        return "Fail to read from the file";
    case static_cast<unsigned int>(StorageError::write_file_error):
        return "Fail to write to the file";
    case static_cast<unsigned int>(StorageError::flush_error):
        return "Fail to flush the write buffer to the file";
    case static_cast<unsigned int>(StorageError::close_file_error):
        return "Fail to close the file";
    case static_cast<unsigned int>(StorageError::create_directory_error):
        return "Fail to create the directory";
    case static_cast<unsigned int>(StorageError::move_file_error):
        return "Fail to move the file";
    case static_cast<unsigned int>(StorageError::remove_file_error):
        return "Fail to remove the file";
    case static_cast<unsigned int>(StorageError::create_iterator_error):
        return "Fail to create an iterator for the directory";
    case static_cast<unsigned int>(StorageError::iterator_next_error):
        return "Fail to query the next entry";
    case static_cast<unsigned int>(StorageError::iterator_remove_error):
        return "Fail to remove the file pointed by the iterator";
    case static_cast<unsigned int>(StorageError::destroy_iterator_error):
        return "Fail to destroy the directory iterator";
    default:
        return "Other error";
    }
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

} // namespace sirius::contract::storage
