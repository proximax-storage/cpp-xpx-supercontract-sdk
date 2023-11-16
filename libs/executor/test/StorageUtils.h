/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <optional>
#include <storage/StorageRequests.h>

namespace sirius::contract::test {

storage::StorageState nextState(const storage::StorageState& oldState);

storage::StorageState randomState();

struct BatchModificationStatistics {
    std::optional<bool>   m_success;
    storage::StorageState m_lowerSandboxState;
    std::vector<std::optional<bool>> m_calls;
};

struct StorageInfo {
    storage::StorageState m_state;
    std::optional<BatchModificationStatistics> m_actualBatch;
    std::vector<BatchModificationStatistics> m_historicBatches;
};

}