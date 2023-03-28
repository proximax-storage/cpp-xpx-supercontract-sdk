/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
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

    std::shared_ptr<AsyncQueryCallback<void>> m_onTaskFinishedCallback;

public:

    SynchronizationTask(SynchronizationRequest&& synchronizationRequest,
                        std::shared_ptr<AsyncQueryCallback<void>>&& m_onTaskFinishedCallback,
                        ContractEnvironment& contractEnvironment,
                        ExecutorEnvironment& executorEnvironment);

public:

    // region blockchain event handler

    bool onEndBatchExecutionPublished(const blockchain::PublishedEndBatchExecutionTransactionInfo& info) override;

    // endregion

public:

    void run() override;

    void terminate() override;

private:

    void onStorageUnavailable();

    void onStorageStateSynchronized();
};

}