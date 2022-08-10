/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/Transactions.h"

namespace sirius::contract {

class BlockchainEventHandler {

public:

    virtual ~BlockchainEventHandler() = default;

    virtual void onEndBatchExecutionPublished( PublishedEndBatchExecutionTransactionInfo&& ) = 0;

    virtual void onEndBatchExecutionSingleTransactionPublished( PublishedEndBatchExecutionSingleTransactionInfo&& ) = 0;

    virtual void onEndBatchExecutionFailed( FailedEndBatchExecutionTransactionInfo&& ) = 0;

    virtual void onStorageSynchronized( const ContractKey& contractKey, uint64_t batchIndex ) = 0;
};

}