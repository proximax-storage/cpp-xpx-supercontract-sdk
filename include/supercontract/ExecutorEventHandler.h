/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "Transactions.h"

namespace sirius::contract {

class ExecutorEventHandler {
public:

    virtual ~ExecutorEventHandler() = default;

    virtual void endBatchTransactionIsReady(const EndBatchExecutionTransactionInfo&) = 0;

    virtual void endBatchSingleTransactionIsReady(const EndBatchExecutionSingleTransactionInfo&) = 0;

};

}