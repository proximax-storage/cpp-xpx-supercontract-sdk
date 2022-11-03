/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ContractEnvironmentMock.h"

namespace sirius::contract::test {

ContractEnvironmentMock::ContractEnvironmentMock(ContractKey& contractKey,
                                                 uint64_t automaticExecutionsSCLimit,
                                                 uint64_t automaticExecutionsSMLimit)
        : m_contractKey(contractKey), m_automaticExecutionsSCLimit(automaticExecutionsSCLimit)
        , m_automaticExecutionsSMLimit(automaticExecutionsSMLimit) {}

const ContractKey &ContractEnvironmentMock::contractKey() const { return m_contractKey; }

const DriveKey &ContractEnvironmentMock::driveKey() const { return m_driveKey; }

const std::set<ExecutorKey> &ContractEnvironmentMock::executors() const { return m_executors; }

uint64_t ContractEnvironmentMock::automaticExecutionsSCLimit() const { return m_automaticExecutionsSCLimit; }

uint64_t ContractEnvironmentMock::automaticExecutionsSMLimit() const { return m_automaticExecutionsSMLimit; }

const ContractConfig &ContractEnvironmentMock::contractConfig() const { return m_contractConfig; }
}
