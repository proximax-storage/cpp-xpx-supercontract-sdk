/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "BaseContractTask.h"

namespace sirius::contract {

class BatchExecutionTask : public BaseContractTask {

private:

    Batch m_batch;
    TaskContext& m_taskContext;

    std::optional<CallRequest> m_callRequest;

public:

    BatchExecutionTask( Batch&& batch,
                        TaskContext& taskContext )
            : m_batch( std::move(batch) )
            , m_taskContext( taskContext )
            {}

    void run() override {

    }

    void terminate() override {

    }

    void onCallExecuted( const CallExecutionResult& result ) override {
        if (!m_callRequest || m_callRequest->m_callId != result.m_callId) {
            return;
        }
    }
};

std::unique_ptr<BaseContractTask> createBatchExecutionTask( Batch&& batch, TaskContext& taskContext ) {
    return std::make_unique<BatchExecutionTask>(std::move(batch), taskContext);
}

}