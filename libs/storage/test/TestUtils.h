/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <common/GlobalEnvironment.h>
#include "storage/Storage.h"

namespace sirius::contract::storage::test {

std::optional<std::string> storageAddress();

logging::LoggerConfig getLoggerConfig();

struct ContextHolder {
    std::unique_ptr<storage::Storage> m_storage;
    std::unique_ptr<storage::StorageModification> m_storageModification;
    std::unique_ptr<storage::SandboxModification> m_sandboxModification;
};

}