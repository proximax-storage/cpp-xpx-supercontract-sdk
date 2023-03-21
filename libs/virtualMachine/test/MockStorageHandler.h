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
        std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void readFile(
        uint64_t fileId,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) override;

    void writeFile(
        uint64_t fileId,
        const std::vector<uint8_t>& buffer,
        std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void flush(
        uint64_t fileId,
        std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void closeFile(
        uint64_t fileId,
        std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void createFSIterator(
        const std::string& path,
        bool recursive,
        std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void hasNextIterator(
        uint64_t iteratorID,
        std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void nextIterator(
        uint64_t iteratorId,
        std::shared_ptr<AsyncQueryCallback<storage::DirectoryIteratorInfo>> callback) override;

    void removeFileIterator(
        uint64_t iteratorId,
        std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void destroyFSIterator(
        uint64_t iteratorId,
        std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void pathExist(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void isFile(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void createDir(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void moveFile(
        const std::string& oldPath,
        const std::string& newPath,
        std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void removeFsEntry(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<void>> callback) override;
};

} // namespace sirius::contract::vm::test