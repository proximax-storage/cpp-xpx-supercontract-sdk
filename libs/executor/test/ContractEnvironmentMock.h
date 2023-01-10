/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "ContractEnvironment.h"
#include "ExecutorEnvironment.h"

namespace sirius::contract::test {

class ContractEnvironmentMock : public ContractEnvironment {
private:
    ContractKey m_contractKey;
    std::map<ExecutorKey, ExecutorInfo> m_executors;
    uint64_t m_automaticExecutionsSCLimit;
    uint64_t m_automaticExecutionsSMLimit;
    ContractConfig m_contractConfig;
    ProofOfExecution m_proofOfExecution;

public:
    DriveKey m_driveKey;
    ContractEnvironmentMock(ExecutorEnvironment& executorEnvironment,
                            ContractKey& contractKey,
                            uint64_t automaticExecutionsSCLimit,
                            uint64_t automaticExecutionsSMLimit);

    ContractEnvironmentMock(ExecutorEnvironment& executorEnvironment,
                            ContractKey& contractKey,
                            uint64_t automaticExecutionsSCLimit,
                            uint64_t automaticExecutionsSMLimit,
                            ContractConfig config);

    const ContractKey& contractKey() const override;

    const DriveKey& driveKey() const override;

    const std::map<ExecutorKey, ExecutorInfo>& executors() const override;

    uint64_t automaticExecutionsSCLimit() const override;

    uint64_t automaticExecutionsSMLimit() const override;

    const ContractConfig& contractConfig() const override;

    void finishTask() override {}

    void addSynchronizationTask() override;

    void delayBatchExecution(Batch batch) override {}

    void cancelBatchesUpTo(uint64_t index) override;

    ProofOfExecution& proofOfExecution() override;

};
}