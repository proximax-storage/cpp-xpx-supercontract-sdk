/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <cstdint>

namespace sirius::contract::vm {

struct CallExecutionResult {
    bool m_success;
    uint32_t m_return;
    uint64_t m_scConsumed;
    uint64_t m_smConsumed;
    // m_proofOfExecution
};

}