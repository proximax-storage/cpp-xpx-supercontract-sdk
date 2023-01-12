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

public:

    AutomaticExecutionBlockchainQueryHandler(ExecutorEnvironment& executorEnvironment,
                                              ContractEnvironment& contractEnvironment,
                                              const CallerKey& callerKey,
                                              uint64_t blockHeight);

    void callerPublicKey(std::shared_ptr<AsyncQueryCallback<CallerKey>> callback) override;

};

}