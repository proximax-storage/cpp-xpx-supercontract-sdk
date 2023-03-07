/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "ContractEnvironment.h"
#include "ExecutorEnvironment.h"
#include "BatchesManagerMock.h"

namespace sirius::contract::test {

class ContractEnvironmentMock : public ContractEnvironment {
private:
    ContractKey m_contractKey;
    uint64_t m_automaticExecutionsSCLimit;
    uint64_t m_automaticExecutionsSMLimit;
    ProofOfExecution m_proofOfExecution;
    ContractConfig m_contractConfig;

public:

    std::map<ExecutorKey, ExecutorInfo> m_executors;

public:

    std::promise<void> m_synchronizationPromise;
    std::unique_ptr<BaseBatchesManager> m_batchesManager;

public:
    DriveKey m_driveKey;
    ContractEnvironmentMock(ExecutorEnvironment& executorEnvironment,
                            ContractKey& contractKey,
                            uint64_t automaticExecutionsSCLimit,
                            uint64_t automaticExecutionsSMLimit,
                            ContractConfig contractConfig = ContractConfig(),
                            std::unique_ptr<BaseBatchesManager> batchesManager = std::make_unique<BatchesManagerMock>());

    const ContractKey& contractKey() const override;

    const DriveKey& driveKey() const override;

    const std::map<ExecutorKey, ExecutorInfo>& executors() const override;

    uint64_t automaticExecutionsSCLimit() const override;

    uint64_t automaticExecutionsSMLimit() const override;

    const ContractConfig& contractConfig() const override;

    void addSynchronizationTask() override;

    void notifyHasNextBatch() override;

    BaseBatchesManager& batchesManager() override;

    ProofOfExecution& proofOfExecution() override;

};
}