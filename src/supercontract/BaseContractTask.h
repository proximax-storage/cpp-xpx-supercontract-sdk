/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "types.h"
#include "contract/Requests.h"
#include "BatchesManager.h"
#include "TaskContext.h"
#include "VirtualMachineEventHandler.h"

namespace sirius::contract {

class BaseContractTask {
public:

    virtual ~BaseContractTask() = default;

    virtual void run() = 0;

    virtual void terminate() = 0;

    virtual void onCallExecuted( const CallExecutionResult& result ) = 0;

};

//std::unique_ptr<BaseContractTask> createInitContractTask(  )
std::unique_ptr<BaseContractTask> createBatchExecutionTask( Batch&& batch, TaskContext& taskContext );

}