/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

namespace sirius::contract {

struct ApplyStorageModificationResponse {
    Hash256     m_batchId;
    Hash256     m_rootHash;
    uint64_t    m_usedSize;
    uint64_t    m_metafilesSize;
    uint64_t    m_fsTreeSize;
};

}