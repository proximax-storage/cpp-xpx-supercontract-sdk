/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BaseContractTask.h"

namespace sirius::contract {

BaseContractTask::BaseContractTask(ExecutorEnvironment& executorEnvironment,
                                   ContractEnvironment& contractEnvironment)
        : m_executorEnvironment(executorEnvironment)
        , m_contractEnvironment(contractEnvironment) {}

ModificationId BaseContractTask::storageModificationId(uint64_t batchId) {
    crypto::Sha3_256_Builder hashBuilder;
    hashBuilder.update(m_contractEnvironment.contractKey());
    hashBuilder.update(utils::RawBuffer{reinterpret_cast<const uint8_t*>(&batchId), sizeof(batchId)});
    ModificationId modificationId;
    hashBuilder.final(modificationId);
    return modificationId;
}

}