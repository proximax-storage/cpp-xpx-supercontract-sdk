/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/Requests.h"
#include "supercontract/AsyncQuery.h"
#include "virtualMachine/CallExecutionResult.h"
#include "VirtualMachineInternetQueryHandler.h"
#include "VirtualMachineBlockchainQueryHandler.h"

#include <memory>

namespace sirius::contract::vm {

class VirtualMachine {

public:

    virtual ~VirtualMachine() = default;

    virtual void executeCall(const ContractKey&,
                             const CallRequest&,
                             std::weak_ptr<VirtualMachineInternetQueryHandler>&& internetQueryHandler,
                             std::weak_ptr<VirtualMachineBlockchainQueryHandler>&& blockchainQueryHandler,
                             std::shared_ptr<AsyncQueryCallback<std::optional<CallExecutionResult>>> callback) = 0;

};


}