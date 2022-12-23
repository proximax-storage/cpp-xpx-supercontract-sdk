/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <virtualMachine/VirtualMachineErrorCode.h>

namespace sirius::contract::vm {

const char* VirtualMachineErrorCategory::name() const noexcept {
    return "vm";
}

std::string VirtualMachineErrorCategory::message(int ev) const {
    switch (ev) {
        case static_cast<unsigned int>(VirtualMachineError::vm_unavailable):
            return "Virtual Machine is unavailable";
        default:
            return "Other error";
    }
}

bool VirtualMachineErrorCategory::equivalent(int code, const std::error_condition& condition) const noexcept {
    return false;
}

const std::error_category& storageErrorCategory() {
    static VirtualMachineErrorCategory instance;
    return instance;
}

std::error_code make_error_code(VirtualMachineError e) {
    return {static_cast<int>(e), storageErrorCategory()};
}

}
