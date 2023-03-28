/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <virtualMachine/VirtualMachineBuilder.h>

namespace sirius::contract::vm {

void VirtualMachineBuilder::setMaxExecutableSizes(const std::map<CallRequest::CallLevel, uint64_t> maxExecutableSizes) {
    m_maxExecutableSizes = maxExecutableSizes;
}

void VirtualMachineBuilder::setStorage(std::weak_ptr<storage::Storage> storage) {
    m_storage = std::move(storage);
}

}