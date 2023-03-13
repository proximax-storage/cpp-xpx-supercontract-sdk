/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <utils/Random.h>
#include "StorageUtils.h"

namespace sirius::contract::test {

storage::StorageState nextState(const storage::StorageState& oldState) {
    return utils::generateRandomByteValue<storage::StorageState>(oldState.m_storageHash);
}

storage::StorageState randomState() {
    return utils::generateRandomByteValue<storage::StorageState>();
}

}