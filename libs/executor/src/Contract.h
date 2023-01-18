/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "executor/ExecutorConfig.h"
#include "supercontract/Identifiers.h"

#include <executor/ExecutorEventHandler.h>
#include <executor/ExecutorInfo.h>
#include <executor/Requests.h>
#include <messenger/Messenger.h>
#include <storage/StorageModifier.h>
#include "ContractMessageEventHandler.h"
#include "ContractBlockchainEventHandler.h"
#include <virtualMachine/VirtualMachine.h>
#include <blockchain/Block.h>
#include "ExecutorEnvironment.h"

namespace sirius::contract {

class Contract
          : public ContractMessageEventHandler
          , public ContractBlockchainEventHandler {
public:

    virtual void addManualCall(const CallRequestParameters& ) = 0;

    virtual void setAutomaticExecutionsEnabledSince(const std::optional<uint64_t>& blockHeight) = 0;

    virtual void removeContract(const RemoveRequest&) = 0;

    virtual void setExecutors(std::map<ExecutorKey, ExecutorInfo>&& executors) = 0;

    virtual void terminate() = 0;

};

}