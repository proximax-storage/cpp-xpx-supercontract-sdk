/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ExecutorEventHandlerMock.h"


namespace sirius::contract::test {

void ExecutorEventHandlerMock::endBatchTransactionIsReady(const blockchain::SuccessfulEndBatchExecutionTransactionInfo&) {

}

void ExecutorEventHandlerMock::endBatchTransactionIsReady(const blockchain::UnsuccessfulEndBatchExecutionTransactionInfo&) {

}

void ExecutorEventHandlerMock::endBatchSingleTransactionIsReady(const blockchain::EndBatchExecutionSingleTransactionInfo&) {

}

void ExecutorEventHandlerMock::synchronizationSingleTransactionIsReady(const blockchain::SynchronizationSingleTransactionInfo& info) {

}

void ExecutorEventHandlerMock::releasedTransactionsAreReady(const blockchain::SerializedAggregatedTransaction& payloads) {

}

} // namespace sirius::contract::test