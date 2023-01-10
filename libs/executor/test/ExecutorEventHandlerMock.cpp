/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ExecutorEventHandlerMock.h"


namespace sirius::contract::test {

void ExecutorEventHandlerMock::endBatchTransactionIsReady(const EndBatchExecutionTransactionInfo&) {

}

void ExecutorEventHandlerMock::endBatchSingleTransactionIsReady(const EndBatchExecutionSingleTransactionInfo&) {

}

void
ExecutorEventHandlerMock::synchronizationSingleTransactionIsReady(const SynchronizationSingleTransactionInfo& info) {

}
} // namespace sirius::contract::test