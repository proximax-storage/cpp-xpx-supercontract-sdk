/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "executor.pb.h"
#include <executor/ExecutorEventHandler.h>

namespace sirius::contract::executor {

class RPCExecutorClientEventHandler: public ExecutorEventHandler {

public:
    void endBatchTransactionIsReady(const blockchain::SuccessfulEndBatchExecutionTransactionInfo& info) override;

    void endBatchTransactionIsReady(const blockchain::UnsuccessfulEndBatchExecutionTransactionInfo& info) override;

    void endBatchSingleTransactionIsReady(const blockchain::EndBatchExecutionSingleTransactionInfo& info) override;

    void synchronizationSingleTransactionIsReady(const blockchain::SynchronizationSingleTransactionInfo& info) override;

    void releasedTransactionsAreReady(const blockchain::SerializedAggregatedTransaction& transaction) override;

};

}