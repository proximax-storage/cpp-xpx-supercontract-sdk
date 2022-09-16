/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "virtualMachine/RPCVirtualMachineBuilder.h"
#include "RPCVirtualMachine.h"

namespace sirius::contract::vm {

std::shared_ptr<VirtualMachine>
RPCVirtualMachineBuilder::build(const StorageObserver& storageObserver,
                                GlobalEnvironment& environment,
                                const std::string& serverAddress) {

    return std::make_shared<RPCVirtualMachine>(storageObserver, environment, serverAddress);
}

}