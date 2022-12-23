/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "executor/ExecutorEventHandler.h"

namespace sirius::contract::test {

    class ExecutorEventHandlerMock : public ExecutorEventHandler {
    public:
        void endBatchTransactionIsReady(const EndBatchExecutionTransactionInfo&) override {}

        void endBatchSingleTransactionIsReady(const EndBatchExecutionSingleTransactionInfo&) override {}
    };
} // namespace sirius::contract::test