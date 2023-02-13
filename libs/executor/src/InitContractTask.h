/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "BaseContractTask.h"

namespace sirius::contract {

class InitContractTask
        : public BaseContractTask {

private:

    const AddContractRequest m_request;

    std::shared_ptr<AsyncQuery> m_storageActualModificationIdQuery;
    Timer                       m_storageActualModificationIdTimer;

    std::shared_ptr<AsyncQueryCallback<void>> m_onTaskFinishedCallback;

public:

    InitContractTask(
            AddContractRequest&& request,
            std::shared_ptr<AsyncQueryCallback<void>>&& onTaskFinishedCallback,
            ContractEnvironment& contractEnvironment,
            ExecutorEnvironment& executorEnvironment);

public:

    void run() override;


    void terminate() override;

public:

    // region blockchain event handler

    bool onEndBatchExecutionPublished(const PublishedEndBatchExecutionTransactionInfo& info) override;

    // endregion

private:

    void requestActualModificationId();

    void onActualModificationIdReceived(const expected<ModificationId>& res);

};

}
