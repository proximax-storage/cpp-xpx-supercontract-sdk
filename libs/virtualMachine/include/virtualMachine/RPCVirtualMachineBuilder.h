/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <virtualMachine/VirtualMachine.h>
#include "storage/StorageObserver.h"

namespace sirius::contract::vm {

class RPCVirtualMachineBuilder {

public:

    std::shared_ptr<VirtualMachine> build(std::weak_ptr<storage::StorageObserver> storageObserver,
                                          GlobalEnvironment& environment,
                                          const std::string& serverAddress);

};

}