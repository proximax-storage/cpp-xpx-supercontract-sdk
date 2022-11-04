/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "../../src/supercontract/ContractEnvironment.h"

namespace sirius::contract::test {

class ContractEnvironmentMock : public ContractEnvironment {
private:
    ContractKey m_contractKey;
    DriveKey m_driveKey;
    std::set<ExecutorKey> m_executors;
    uint64_t m_automaticExecutionsSCLimit;
    uint64_t m_automaticExecutionsSMLimit;
    ContractConfig m_contractConfig;

public:
    ContractEnvironmentMock(ContractKey& contractKey,
                            uint64_t automaticExecutionsSCLimit,
                            uint64_t automaticExecutionsSMLimit);

    const ContractKey& contractKey() const override;

    const DriveKey& driveKey() const override;

    const std::set<ExecutorKey>& executors() const override;

    uint64_t automaticExecutionsSCLimit() const override;

    uint64_t automaticExecutionsSMLimit() const override;

    const ContractConfig& contractConfig() const override;

    void finishTask() override {}

    void addSynchronizationTask() override {}

    void delayBatchExecution(Batch batch) override {}
};
}