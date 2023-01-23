/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <virtualMachine/RPCVirtualMachineBuilder.h>
#include "RPCVirtualMachine.h"

namespace sirius::contract::vm {

RPCVirtualMachineBuilder::RPCVirtualMachineBuilder(
        const std::string& address)
        : m_address(address) {}

std::shared_ptr<VirtualMachine> RPCVirtualMachineBuilder::build(GlobalEnvironment& environment) {
    return std::make_shared<RPCVirtualMachine>(m_storageContentObserver, environment, m_address);
}

}