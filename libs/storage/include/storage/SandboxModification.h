/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <supercontract/AsyncQuery.h>

#include "StorageRequests.h"

namespace sirius::contract::storage {

enum class OpenFileMode {
    READ,
    WRITE
};

class SandboxModification {

public:

    virtual ~SandboxModification() = default;

    virtual void applySandboxModifications(bool success,
                                           std::shared_ptr<AsyncQueryCallback<SandboxModificationDigest>> callback) = 0;

    virtual void openFile(const std::string& path, OpenFileMode mode,
                          std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) = 0;

    virtual void writeFile(uint64_t fileId, const std::vector<uint8_t>& buffer,
                           std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void readFile(uint64_t fileId, uint64_t bytesToRead,
                          std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) = 0;

    virtual void closeFile(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void flush(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void
    createDirectories(const std::string& path,
                      std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void directoryIteratorCreate(const std::string& path, bool recursive,
                                         std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) = 0;

    virtual void directoryIteratorHasNext(uint64_t id,
                                          std::shared_ptr<AsyncQueryCallback<bool>> callback) = 0;

    virtual void directoryIteratorNext(uint64_t id,
                                       std::shared_ptr<AsyncQueryCallback<std::string>> callback) = 0;

    virtual void directoryIteratorDestroy(uint64_t id,
                                          std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void removeFilesystemEntry(const std::string& path,
                                       std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void moveFilesystemEntry(const std::string& src, const std::string& dst,
                                     std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void pathExist(const std::string& path,
                           std::shared_ptr<AsyncQueryCallback<bool>> callback) = 0;

    virtual void isFile(const std::string& path,
                        std::shared_ptr<AsyncQueryCallback<bool>> callback) = 0;
};

}