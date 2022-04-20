/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BaseContractTask.h"

namespace sirius::contract {

class InitContractTask : BaseContractTask {

private:

    const AddContractRequest m_request;

    bool isSynchronized = false;

public:

    InitContractTask(
            const AddContractRequest& request,
            TaskContext& taskContext )
            : BaseContractTask( taskContext )
            , m_request(request)
            {}

public:

    void run() override {

        // Now the task does not do anything,
        // but it can do something in future

        if (!m_request.isInActualState) {
            m_taskContext.notifyNeedsSynchronization();
        }
        else {
            m_taskContext.onTaskFinished();
        }
    }


    void terminate() override {
        m_taskContext.onTaskFinished();
    }

};

}