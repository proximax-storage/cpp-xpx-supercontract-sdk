/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "executor/ExecutorConfig.h"
#include <common/Identifiers.h>

#include <executor/ExecutorEventHandler.h>
#include <executor/ExecutorInfo.h>
#include <executor/Requests.h>
#include <messenger/Messenger.h>
#include "ContractMessageEventHandler.h"
#include "ContractBlockchainEventHandler.h"
#include <virtualMachine/VirtualMachine.h>
#include <blockchain/Block.h>
#include <executor/ManualCallRequest.h>
#include "ExecutorEnvironment.h"

namespace sirius::contract {

class Contract
        : public ContractMessageEventHandler, public ContractBlockchainEventHandler {
public:

    virtual void addManualCall(const ManualCallRequest& request) = 0;

    virtual void setAutomaticExecutionsEnabledSince(uint64_t blockHeight) = 0;

    virtual void removeContract(const RemoveRequest&, std::shared_ptr<AsyncQueryCallback<void>>&& callback) = 0;

    virtual void setExecutors(std::map<ExecutorKey, ExecutorInfo>&& executors) = 0;

    virtual void terminate() = 0;

};

}