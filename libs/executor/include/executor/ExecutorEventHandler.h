/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <blockchain/AggregatedTransaction.h>
#include <blockchain/SuccessfulEndBatchExecutionTransaction.h>
#include <blockchain/UnsuccessfulEndBatchExecutionTransaction.h>
#include <blockchain/EndBatchExecutionSingleTransaction.h>
#include <blockchain/SynchronizationSingleTransaction.h>

namespace sirius::contract {

class ExecutorEventHandler {
public:

    virtual ~ExecutorEventHandler() = default;

    virtual void endBatchTransactionIsReady(const blockchain::SuccessfulEndBatchExecutionTransactionInfo&) = 0;

    virtual void endBatchTransactionIsReady(const blockchain::UnsuccessfulEndBatchExecutionTransactionInfo&) = 0;

    virtual void endBatchSingleTransactionIsReady(const blockchain::EndBatchExecutionSingleTransactionInfo&) = 0;

    virtual void synchronizationSingleTransactionIsReady(const blockchain::SynchronizationSingleTransactionInfo&) = 0;

    virtual void releasedTransactionsAreReady(const blockchain::SerializedAggregatedTransaction&) = 0;
};

}