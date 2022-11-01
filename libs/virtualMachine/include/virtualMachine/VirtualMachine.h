/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "VirtualMachineBlockchainQueryHandler.h"
#include "VirtualMachineInternetQueryHandler.h"
#include "VirtualMachineStorageQueryHandler.h"
#include "supercontract/AsyncQuery.h"
#include "supercontract/Requests.h"
#include <memory>
#include <virtualMachine/CallRequest.h>

namespace sirius::contract::vm {

class VirtualMachine {

public:
    virtual ~VirtualMachine() = default;

    virtual void executeCall(const CallRequest&,
                             std::weak_ptr<VirtualMachineInternetQueryHandler> internetQueryHandler,
                             std::weak_ptr<VirtualMachineBlockchainQueryHandler> blockchainQueryHandler,
                             std::weak_ptr<VirtualMachineStorageQueryHandler> storageQueryHandler,
                             std::shared_ptr<AsyncQueryCallback<CallExecutionResult>> callback) = 0;
};

} // namespace sirius::contract::vm