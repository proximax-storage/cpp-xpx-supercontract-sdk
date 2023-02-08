/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <storage/RPCStorageBuilder.h>
#include "RPCStorage.h"

namespace sirius::contract::storage {

RPCStorageBuilder::RPCStorageBuilder(const std::string& address): m_address(address) {}

std::shared_ptr<Storage> RPCStorageBuilder::build(GlobalEnvironment& environment) {
    return std::make_shared<RPCStorage>(environment, m_address);
}
}