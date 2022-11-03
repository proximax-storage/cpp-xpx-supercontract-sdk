/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <storage/StorageErrorCode.h>
#include <internet/InternetErrorCode.h>
#include <virtualMachine/ExecutionErrorConidition.h>
#include <virtualMachine/VirtualMachineErrorCode.h>

namespace sirius::contract::vm {

const char* ExecutionErrorCategory::name() const noexcept {
    return "supercontract";
}

std::string ExecutionErrorCategory::message(int ev) const {
    switch (ev) {
        case static_cast<unsigned int>(ExecutionError::virtual_machine_unavailable):
            return "Virtual Machine is unavailable";
        case static_cast<unsigned int>(ExecutionError::storage_unavailable):
            return "Storage is unavailable";
        case static_cast<unsigned int>(ExecutionError::internet_unavailable):
            return "Internet is unavailable";
        case static_cast<unsigned int>(ExecutionError::internet_incorrect_query):
            return "Incorrect internet query";
        case static_cast<unsigned int>(ExecutionError::storage_incorrect_query):
            return "Incorrect storage query";
        default:
            return "Other error";
    }
}

bool ExecutionErrorCategory::equivalent(const std::error_code& code, int condition) const noexcept {
    switch (condition) {
        case static_cast<int>(ExecutionError::virtual_machine_unavailable):
            return code == VirtualMachineError::vm_unavailable;
        case static_cast<int>(ExecutionError::storage_unavailable):
            return code == storage::StorageError::storage_unavailable;
        case static_cast<int>(ExecutionError::storage_incorrect_query):
            return code.category() == storage::storageErrorCategory() &&
                   code != storage::StorageError::storage_unavailable;
        case static_cast<int>(ExecutionError::internet_unavailable):
            return code == internet::InternetError::internet_unavailable;
        case static_cast<int>(ExecutionError::internet_incorrect_query):
            return code.category() == internet::internetErrorCategory() &&
                   code != internet::InternetError::internet_unavailable;
        default:
            return false;
    }
}

const std::error_category& executionErrorCategory() {
    static ExecutionErrorCategory instance;
    return instance;
}

std::error_condition make_error_condition(ExecutionError e) {
    return {static_cast<int>(e), executionErrorCategory()};
}

} // namespace sirius::contract::vm