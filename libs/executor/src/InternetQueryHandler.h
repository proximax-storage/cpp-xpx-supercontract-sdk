/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "ContractEnvironment.h"
#include "ExecutorEnvironment.h"
#include <internet/InternetConnection.h>
#include <common/AsyncQuery.h>
#include <common/Identifiers.h>
#include <virtualMachine/VirtualMachineInternetQueryHandler.h>

namespace sirius::contract {

class InternetQueryHandler
        : private SingleThread,
        public vm::VirtualMachineInternetQueryHandler {

private:
    ExecutorEnvironment& m_executorEnvironment;
    ContractEnvironment& m_contractEnvironment;

    const CallId m_callId;

    uint16_t m_maxConnections;

    std::shared_ptr<AsyncQuery> m_asyncQuery;

    uint64_t totalConnectionsCreated = 0;
    std::map<uint64_t, internet::InternetConnection> m_internetConnections;

public:
    InternetQueryHandler(const CallId& callId,
                         uint16_t maxConnections,
                         ExecutorEnvironment& executorEnvironment,
                         ContractEnvironment& contractEnvironment);

    void
    openConnection(const std::string& url,
                   bool softRevocationMode,
                   std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void read(uint64_t connectionId,
              std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) override;

    void closeConnection(uint64_t connectionId,
                         std::shared_ptr<AsyncQueryCallback<void>> callback) override;
};

} // namespace sirius::contract