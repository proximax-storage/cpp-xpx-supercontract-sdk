/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "contract/Transactions.h"

namespace sirius::contract {

class ContractBlockchainEventHandler {

public:

    virtual ~ContractBlockchainEventHandler() = default;

    virtual bool onEndBatchExecutionPublished( const PublishedEndBatchExecutionTransactionInfo& ) {
        return false;
    }

    virtual bool onEndBatchExecutionSingleTransactionPublished( const PublishedEndBatchExecutionSingleTransactionInfo& ) {
        return false;
    }

    virtual bool onEndBatchExecutionFailed( const FailedEndBatchExecutionTransactionInfo& ) {
        return false;
    }

    virtual bool onStorageSynchronized( uint64_t batchIndex ) {
        return false;
    }
};

}