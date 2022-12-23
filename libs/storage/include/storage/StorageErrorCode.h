/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <system_error>

namespace sirius::contract::storage {

enum class StorageError {
    storage_unavailable = 1,
    open_file_error,
    read_file_error,
    write_file_error,
    flush_error,
    close_file_error,
    create_directory_error,
    move_file_error,
    remove_file_error,
    create_iterator_error,
    iterator_next_error,
    iterator_has_next_error,
    iterator_remove_error,
    destroy_iterator_error,
    path_exist_error,
    is_file_error,
};

class StorageErrorCategory
        : public std::error_category {
public:
    const char* name() const noexcept override;

    std::string message(int ev) const override;

    bool equivalent(int code, const std::error_condition& condition) const noexcept override;
};

const std::error_category& storageErrorCategory();

std::error_code make_error_code(StorageError e);

}

namespace std {

template<>
struct is_error_code_enum<sirius::contract::storage::StorageError>
        : public true_type {
};

}
