/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/AsyncQuery.h"
#include <storage/StorageErrorCode.h>
#include <optional>
#include <string>

namespace sirius::contract::vm {

class VirtualMachineStorageQueryHandler {

public:
    virtual void openFile(
        const std::string& path,
        const std::string& mode,
        std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual void readFile(
        uint64_t fileId,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual void writeFile(
        uint64_t fileId,
        const std::vector<uint8_t>& buffer,
        std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual void flush(
        uint64_t fileId,
        std::shared_ptr<AsyncQueryCallback<void>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual void closeFile(
        uint64_t fileId,
        std::shared_ptr<AsyncQueryCallback<void>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual void createFSIterator(
        const std::string& path,
        bool recursive,
        std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual void hasNextIterator(
        uint64_t iteratorID,
        std::shared_ptr<AsyncQueryCallback<bool>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual void nextIterator(
        uint64_t iteratorId,
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual void removeFileIterator(
        uint64_t iteratorId,
        std::shared_ptr<AsyncQueryCallback<void>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual void destroyFSIterator(
        uint64_t iteratorId,
        std::shared_ptr<AsyncQueryCallback<void>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual void pathExist(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<bool>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual void isFile(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<bool>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual void createDir(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<void>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual void moveFile(
        const std::string& oldPath,
        const std::string& newPath,
        std::shared_ptr<AsyncQueryCallback<void>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual void removeFsEntry(
        const std::string& path,
        std::shared_ptr<AsyncQueryCallback<void>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(storage::make_error_code(storage::StorageError::incorrect_query)));
    }

    virtual ~VirtualMachineStorageQueryHandler() = default;
};

} // namespace sirius::contract::vm