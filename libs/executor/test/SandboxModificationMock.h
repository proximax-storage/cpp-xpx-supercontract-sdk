/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "storage/SandboxModification.h"
#include "StorageUtils.h"
#include "utils/Random.h"

namespace sirius::contract::test {

class SandboxModificationMock: public storage::SandboxModification {

private:

    std::shared_ptr<StorageInfo> m_info;

public:

    SandboxModificationMock(std::shared_ptr<StorageInfo> statistics);

    void applySandboxModification(bool success,
                                   std::shared_ptr<AsyncQueryCallback<storage::SandboxModificationDigest>> callback) override;

    void openFile(const std::string& path, storage::OpenFileMode mode,
                  std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void writeFile(uint64_t fileId, const std::vector<uint8_t>& buffer,
                   std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void readFile(uint64_t fileId, uint64_t bytesToRead,
                  std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) override;

    void closeFile(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void flush(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void createDirectories(const std::string& path, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void directoryIteratorCreate(const std::string& path, bool recursive,
                                 std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void directoryIteratorHasNext(uint64_t id, std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void directoryIteratorNext(uint64_t id, std::shared_ptr<AsyncQueryCallback<storage::DirectoryIteratorInfo>> callback) override;

    void directoryIteratorDestroy(uint64_t id, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void removeFilesystemEntry(const std::string& path, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void moveFilesystemEntry(const std::string& src, const std::string& dst,
                             std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void pathExist(const std::string& path, std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void isFile(const std::string& path, std::shared_ptr<AsyncQueryCallback<bool>> callback) override;
};

}

