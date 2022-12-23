/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "BaseContractTask.h"

namespace sirius::contract {

class SynchronizationTask
        : public BaseContractTask {

private:

    const SynchronizationRequest m_request;

    std::shared_ptr<AsyncQuery> m_storageQuery;

    Timer m_storageTimer;

public:

    SynchronizationTask(SynchronizationRequest&& synchronizationRequest,
                        ContractEnvironment& contractEnvironment,
                        ExecutorEnvironment& executorEnvironment);

public:

    // region blockchain event handler

    bool onEndBatchExecutionPublished(const PublishedEndBatchExecutionTransactionInfo& info) override;

    bool onStorageSynchronizedPublished(uint64_t) override;

    // endregion

public:

    void run() override;

    void terminate() override;

private:

    void onStorageUnavailable();

    void onStorageStateSynchronized();
};

}