/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BaseContractTask.h"

namespace sirius::contract {

class RemoveContractTask
        : public BaseContractTask {

private:

    const RemoveRequest m_request;
    
    std::shared_ptr<AsyncQueryCallback<void>> m_onTaskFinishedCallback;
    
public:

    RemoveContractTask(
            RemoveRequest&& request,
            std::shared_ptr<AsyncQueryCallback<void>>&& onTaskFinishedCallback,
            ContractEnvironment& contractEnvironment,
            ExecutorEnvironment& executorEnvironment);

public:

    void run() override;

    void terminate() override;

};

}