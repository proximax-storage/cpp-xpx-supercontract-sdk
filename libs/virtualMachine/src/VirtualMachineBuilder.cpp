/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <virtualMachine/VirtualMachineBuilder.h>

namespace sirius::contract::vm {

void VirtualMachineBuilder::setContentObserver(std::weak_ptr<storage::StorageObserver> storageObserver) {
    m_storageContentObserver = std::move(storageObserver);
}

}