/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <optional>
#include <string>
#include <virtualMachine/VirtualMachineStorageQueryHandler.h>

namespace sirius::contract::vm::test {

class MockStorageHandler : public VirtualMachineStorageQueryHandler {

public:
    virtual void openFile(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) = 0;

    virtual void read(
        uint64_t fileId,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) = 0;

    virtual void write(
        uint64_t fileId,
        std::vector<uint8_t> buffer,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) = 0;

    virtual void flush(
        uint64_t fileId,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) = 0;

    virtual void closeFile(
        uint64_t fileId,
        std::shared_ptr<AsyncQueryCallback<void>> callback) = 0;

    virtual void createFSIterator(
        const std::string& path,
        bool recursive,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) = 0;

    virtual void hasNext(
        uint64_t iteratorID,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) = 0;

    virtual void next(
        uint64_t iteratorID,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) = 0;

    virtual void destroyFSIterator(
        uint64_t iteratorID,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) = 0;

    virtual void pathExist(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) = 0;

    virtual void isFile(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) = 0;

    virtual void createDir(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) = 0;

    virtual void moveFile(
        const std::string& oldPath,
        const std::string& newPath,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) = 0;

    virtual void removeFile(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) = 0;

    virtual ~MockStorageHandler() = default;
};

} // namespace sirius::contract::vm::test