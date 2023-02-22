/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "Transactions.h"
#include <blockchain/EmbeddedTransaction.h>

namespace sirius::contract {

class ExecutorEventHandler {
public:

    virtual ~ExecutorEventHandler() = default;

    virtual void endBatchTransactionIsReady(const SuccessfulEndBatchExecutionTransactionInfo&) = 0;

    virtual void endBatchTransactionIsReady(const UnsuccessfulEndBatchExecutionTransactionInfo&) = 0;

    virtual void endBatchSingleTransactionIsReady(const EndBatchExecutionSingleTransactionInfo&) = 0;

    virtual void synchronizationSingleTransactionIsReady(const SynchronizationSingleTransactionInfo&) = 0;

    virtual void releasedTransactionsAreReady(const blockchain::SerializedAggregatedTransaction&) = 0;
};

}