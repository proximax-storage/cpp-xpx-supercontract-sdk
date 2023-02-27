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

class BlockchainEventHandler {

public:

    virtual ~BlockchainEventHandler() = default;

    virtual void onEndBatchExecutionPublished(blockchain::PublishedEndBatchExecutionTransactionInfo&&) = 0;

    virtual void onEndBatchExecutionSingleTransactionPublished(blockchain::PublishedEndBatchExecutionSingleTransactionInfo&&) = 0;

    virtual void onEndBatchExecutionFailed(blockchain::FailedEndBatchExecutionTransactionInfo&&) = 0;

    virtual void onStorageSynchronizedPublished(blockchain::PublishedSynchronizeSingleTransactionInfo&&) = 0;
};

}