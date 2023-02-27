/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <cstdint>

namespace sirius::contract::blockchain {

struct CallExecutorParticipation {
    uint64_t m_scConsumed = 0;
    uint64_t m_smConsumed = 0;

    template<class Archive>
    void serialize(Archive& arch) {
        arch(m_scConsumed);
        arch(m_smConsumed);
    }
};

}