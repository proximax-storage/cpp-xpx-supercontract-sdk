/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <storage/StorageModification.h>
#include "RPCClient.h"

namespace sirius::contract::storage {

class RPCStorageModification : public StorageModification, private SingleThread {

private:

    GlobalEnvironment& m_environment;
    std::weak_ptr<RPCClient> m_pRPCClient;
    DriveKey m_driveKey;

public:

    RPCStorageModification(GlobalEnvironment& m_environment,
                           std::weak_ptr<RPCClient> pRPCClient,
                           const DriveKey& driveKey);

    void initiateSandboxModification(
            const std::vector<std::string>& serviceFolders,
            std::shared_ptr<AsyncQueryCallback<std::unique_ptr<SandboxModification>>> callback) override;

    void evaluateStorageHash(std::shared_ptr<AsyncQueryCallback<StorageState>> callback) override;

    void applyStorageModification(bool success, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

};

}