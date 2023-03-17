/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "AutomaticExecutionBlockchainQueryHandler.h"

namespace sirius::contract {

class ManualCallBlockchainQueryHandler : public AutomaticExecutionsBlockchainQueryHandler {

private:

    TransactionHash m_transactionHash;
    std::vector<ServicePayment> m_servicePayments;

public:

    ManualCallBlockchainQueryHandler(ExecutorEnvironment& executorEnvironment,
                                     ContractEnvironment& contractEnvironment,
                                     const CallerKey& callerKey,
                                     uint64_t blockHeight,
                                     uint64_t executionPayment,
                                     uint64_t downloadPayment,
                                     const TransactionHash& transactionHash,
                                     std::vector<ServicePayment> servicePayments);

    void transactionHash(std::shared_ptr<AsyncQueryCallback<TransactionHash>> callback) override;

    void servicePayments(std::shared_ptr<AsyncQueryCallback<std::vector<ServicePayment>>> callback) override;
};

}