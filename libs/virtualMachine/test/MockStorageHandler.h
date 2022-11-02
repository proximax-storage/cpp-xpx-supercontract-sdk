/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <fstream>
#include <optional>
#include <string>
#include <virtualMachine/VirtualMachineStorageQueryHandler.h>

namespace sirius::contract::vm::test {

class MockStorageHandler : public VirtualMachineStorageQueryHandler {

    std::ifstream m_reader;
    std::ofstream m_writer;
    std::filesystem::directory_iterator m_iterator;
    bool m_recursive;

public:
    MockStorageHandler();

    void openFile(
        const std::string& path,
        const std::string& mode,
        std::shared_ptr<AsyncQueryCallback<uint64_t>> callback);

    void read(
        uint64_t fileId,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback);

    void write(
        uint64_t fileId,
        std::vector<uint8_t> buffer,
        std::shared_ptr<AsyncQueryCallback<uint64_t>> callback);

    void flush(
        uint64_t fileId,
        std::shared_ptr<AsyncQueryCallback<bool>> callback);

    void closeFile(
        uint64_t fileId,
        std::shared_ptr<AsyncQueryCallback<void>> callback);

    void createFSIterator(
        const std::string& path,
        bool recursive,
        std::shared_ptr<AsyncQueryCallback<uint64_t>> callback);

    void hasNext(
        uint64_t iteratorID,
        std::shared_ptr<AsyncQueryCallback<bool>> callback);

    void next(
        uint64_t iteratorID,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback);

    void removeFileIterator(
        uint64_t iteratorID,
        std::shared_ptr<AsyncQueryCallback<bool>> callback);

    void destroyFSIterator(
        uint64_t iteratorID,
        std::shared_ptr<AsyncQueryCallback<void>> callback);

    void pathExist(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<bool>> callback);

    void isFile(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<bool>> callback);

    void createDir(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<bool>> callback);

    void moveFile(
        const std::string& oldPath,
        const std::string& newPath,
        std::shared_ptr<AsyncQueryCallback<bool>> callback);

    void removeFile(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<bool>> callback);

    ~MockStorageHandler() = default;
};

} // namespace sirius::contract::vm::test