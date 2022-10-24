/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/Identifiers.h"

namespace sirius::contract::storage {

struct StorageState {
    StorageHash m_storageHash;
    uint64_t m_usedDriveSize = 0;
    uint64_t m_metaFilesSize = 0;
    uint64_t m_fileStructureSize = 0;
};

struct SandboxModificationDigest {
    bool m_success;
    int64_t m_sandboxSizeDelta;
    int64_t m_stateSizeDelta;
};

}