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
        void endBatchTransactionIsReady(const SuccessfulEndBatchExecutionTransactionInfo&) override;

        void endBatchTransactionIsReady(const UnsuccessfulEndBatchExecutionTransactionInfo&) override;

        void endBatchSingleTransactionIsReady(const EndBatchExecutionSingleTransactionInfo&) override;

        void synchronizationSingleTransactionIsReady(const SynchronizationSingleTransactionInfo& info) override;

        void releasedTransactionsAreReady(const blockchain::SerializedAggregatedTransaction& payloads) override;
    };
} // namespace sirius::contract::test