/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "AutorunBlockchainQueryHandler.h"

namespace sirius::contract {

class AutomaticExecutionBlockchainQueryHandler : public AutorunBlockchainQueryHandler {

protected:

    CallerKey m_caller;
    uint64_t m_executionPayment;
    uint64_t m_downloadPayment;

public:

    AutomaticExecutionBlockchainQueryHandler(ExecutorEnvironment& executorEnvironment,
                                              ContractEnvironment& contractEnvironment,
                                              const CallerKey& callerKey,
                                              uint64_t blockHeight,
                                              uint64_t executionPayment,
                                              uint64_t downloadPayment);

    void callerPublicKey(std::shared_ptr<AsyncQueryCallback<CallerKey>> callback) override;

    void downloadPayment(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void executionPayment(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

};

}