/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <storage/SandboxModification.h>
#include "supercontract/GlobalEnvironment.h"
#include "RPCClient.h"

namespace sirius::contract::storage {

class RPCSandboxModification : public SandboxModification, private SingleThread {

private:

    GlobalEnvironment& m_environment;
    std::weak_ptr<RPCClient> m_pRPCClient;
    DriveKey m_driveKey;

public:

    RPCSandboxModification(GlobalEnvironment& m_environment,
                           std::weak_ptr<RPCClient> pRPCClient,
                           const DriveKey& driveKey);

    void openFile(const std::string& path, OpenFileMode mode,
                  std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void writeFile(uint64_t fileId, const std::vector<uint8_t>& buffer,
                   std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void readFile(uint64_t fileId, uint64_t bytesToRead,
                  std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) override;

    void closeFile(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void flush(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void createDirectories(const std::string& path,
                           std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void directoryIteratorCreate(const std::string& path, bool recursive,
                                 std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void directoryIteratorHasNext(uint64_t id,
                                  std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void directoryIteratorNext(uint64_t id,
                               std::shared_ptr<AsyncQueryCallback<DirectoryIteratorInfo>> callback) override;

    void directoryIteratorDestroy(uint64_t id,
                                  std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void removeFilesystemEntry(const std::string& path,
                               std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void moveFilesystemEntry(const std::string& src, const std::string& dst,
                             std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void pathExist(const std::string& path,
                   std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void isFile(const std::string& path,
                std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void fileSize(const std::string& path, std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void applySandboxModification(bool success,
                                   std::shared_ptr<AsyncQueryCallback<SandboxModificationDigest>> callback) override;
};

}