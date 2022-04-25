/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BaseContractTask.h"

namespace sirius::contract {

class RemoveContractTask : public BaseContractTask {

private:

    const RemoveRequest m_request;

public:

    RemoveContractTask(
            RemoveRequest&& request,
            ContractEnvironment& contractEnvironment,
            ExecutorEnvironment& executorEnvironment)
            : BaseContractTask( executorEnvironment, contractEnvironment )
            , m_request( std::move( request )) {}

public:

    void run() override {
        m_contractEnvironment.finishTask();
    }

    void terminate() override {
        m_contractEnvironment.finishTask();
    }

};

std::unique_ptr<BaseContractTask> createRemoveContractTask( RemoveRequest&& request, ContractEnvironment& contractEnvironment, ExecutorEnvironment& executorEnvironment) {
    return std::make_unique<RemoveContractTask>(std::move(request), contractEnvironment, executorEnvironment);
}

}