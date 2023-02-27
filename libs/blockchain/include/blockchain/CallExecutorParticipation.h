/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <cstdint>

namespace sirius::contract::blockchain {

struct CallExecutorParticipation {
    uint64_t m_scConsumed;
    uint64_t m_smConsumed;

    template<class Archive>
    void serialize(Archive& arch) {
        arch(m_scConsumed);
        arch(m_smConsumed);
    }
};

}