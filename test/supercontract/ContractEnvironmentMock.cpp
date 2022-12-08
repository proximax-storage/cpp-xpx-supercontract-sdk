/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ContractEnvironmentMock.h"

namespace sirius::contract::test {

ContractEnvironmentMock::ContractEnvironmentMock(ExecutorEnvironment& executorEnvironment,
                                                 ContractKey& contractKey,
                                                 uint64_t automaticExecutionsSCLimit,
                                                 uint64_t automaticExecutionsSMLimit)
        : m_contractKey(contractKey)
          , m_automaticExecutionsSCLimit(automaticExecutionsSCLimit)
          , m_automaticExecutionsSMLimit(automaticExecutionsSMLimit)
          , m_proofOfExecution(executorEnvironment, executorEnvironment.keyPair()) {}

ContractEnvironmentMock::ContractEnvironmentMock(ExecutorEnvironment& executorEnvironment,
                                                 ContractKey& contractKey,
                                                 uint64_t automaticExecutionsSCLimit,
                                                 uint64_t automaticExecutionsSMLimit,
                                                 ContractConfig config)
        : m_contractKey(contractKey)
        , m_automaticExecutionsSCLimit(automaticExecutionsSCLimit)
        , m_automaticExecutionsSMLimit(automaticExecutionsSMLimit)
        , m_proofOfExecution(executorEnvironment, executorEnvironment.keyPair())
        , m_contractConfig(config){}

const ContractKey& ContractEnvironmentMock::contractKey() const { return m_contractKey; }

const DriveKey& ContractEnvironmentMock::driveKey() const { return m_driveKey; }

const std::set<ExecutorKey>& ContractEnvironmentMock::executors() const { return m_executors; }

uint64_t ContractEnvironmentMock::automaticExecutionsSCLimit() const { return m_automaticExecutionsSCLimit; }

uint64_t ContractEnvironmentMock::automaticExecutionsSMLimit() const { return m_automaticExecutionsSMLimit; }

const ContractConfig& ContractEnvironmentMock::contractConfig() const { return m_contractConfig; }

void ContractEnvironmentMock::cancelBatchesUpTo(uint64_t index) {}

void ContractEnvironmentMock::addSynchronizationTask() {}

ProofOfExecution& ContractEnvironmentMock::proofOfExecution() {
    return m_proofOfExecution;
}

}