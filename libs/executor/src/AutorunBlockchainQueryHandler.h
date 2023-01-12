/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <virtualMachine/VirtualMachineBlockchainQueryHandler.h>
#include "ExecutorEnvironment.h"
#include "ContractEnvironment.h"
#include <supercontract/SingleThread.h>

namespace sirius::contract {

class AutorunBlockchainQueryHandler:
        protected SingleThread,
        public vm::VirtualMachineBlockchainQueryHandler {

protected:

    ExecutorEnvironment& m_executorEnvironment;
    ContractEnvironment& m_contractEnvironment;

    uint64_t m_height;

    std::shared_ptr<AsyncQuery> m_query;

public:

    AutorunBlockchainQueryHandler(ExecutorEnvironment& executorEnvironment,
                                  ContractEnvironment& contractEnvironment,
                                  uint64_t height);

    void blockHeight(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void blockHash(std::shared_ptr<AsyncQueryCallback<BlockHash>> callback) override;

    void blockTime(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void blockGenerationTime(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

};

}