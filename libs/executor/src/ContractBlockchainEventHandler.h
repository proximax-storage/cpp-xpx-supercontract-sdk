/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <blockchain/PublishedEndBatchExecutionSingleTransactionInfo.h>
#include <blockchain/PublishedEndBatchExecutionTransactionInfo.h>
#include <blockchain/PublishedSynchronizeTransactionInfo.h>
#include <blockchain/FailedEndBatchExecutionTransactionInfo.h>

namespace sirius::contract {

class ContractBlockchainEventHandler {

public:

    virtual ~ContractBlockchainEventHandler() = default;

    virtual bool onEndBatchExecutionPublished(const blockchain::PublishedEndBatchExecutionTransactionInfo&) {
        return false;
    }

    virtual bool
    onEndBatchExecutionSingleTransactionPublished(const blockchain::PublishedEndBatchExecutionSingleTransactionInfo&) {
        return false;
    }

    virtual bool onEndBatchExecutionFailed(const blockchain::FailedEndBatchExecutionTransactionInfo&) {
        return false;
    }

    virtual bool onStorageSynchronizedPublished(uint64_t batchIndex) {
        return false;
    }

    virtual bool onBlockPublished(uint64_t height) {
        return false;
    }
};

}