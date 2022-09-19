/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BaseContractTask.h"

namespace sirius::contract {

BaseContractTask::BaseContractTask(ExecutorEnvironment& executorEnvironment,
                                   ContractEnvironment& contractEnvironment)
        : m_executorEnvironment(executorEnvironment)
        , m_contractEnvironment(contractEnvironment) {}

}