/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "StorageQueryHandler.h"

namespace sirius::contract {

StorageQueryHandler::StorageQueryHandler(const CallId& callId,
                                         ExecutorEnvironment& executorEnvironment,
                                         ContractEnvironment& contractEnvironment,
                                         const std::string& pathPrefix)
                                         : m_callId(callId)
                                         , m_executorEnvironment(executorEnvironment)
                                         , m_contractEnvironment(contractEnvironment)
                                         , m_pathPrefix(pathPrefix) {}

void StorageQueryHandler::openFile(const std::string& path, const std::string& mode,
                                        std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to open a file: {}",
                                        m_callId, path);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[asyncQuery, storageCallback] = createAsyncQuery<uint64_t>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::open_file_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);

    storage::OpenFileMode fileMode;
    if (mode == "w") {
        fileMode = storage::OpenFileMode::WRITE;
    } else {
        fileMode = storage::OpenFileMode::READ;
    }
    storage->openFile(m_contractEnvironment.driveKey(), getPrefixedPath(path), fileMode, storageCallback);
}

void StorageQueryHandler::writeFile(uint64_t fileId, const std::vector<uint8_t>& buffer,
                                         std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to write to file {}",
                                        m_callId, fileId);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        if (!res) {
            callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::write_file_error)));
        }
        callback->postReply(buffer.size());
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::write_file_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->writeFile(m_contractEnvironment.driveKey(), fileId, {buffer.begin(), buffer.end()}, storageCallback);
}

void StorageQueryHandler::readFile(uint64_t fileId,
                                        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to read file {}",
                                        m_callId, fileId);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[asyncQuery, storageCallback] = createAsyncQuery<std::vector<uint8_t>>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::read_file_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->readFile(m_contractEnvironment.driveKey(), fileId, 16 * 1024, storageCallback);
}

void StorageQueryHandler::flush(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to flush a write buffer to file {}",
                                        m_callId, fileId);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
                                                                   m_asyncQuery.reset();
                                                                   callback->postReply(std::forward<decltype(res)>(res));
                                                               }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::flush_error))); },
                                                               m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->flush(m_contractEnvironment.driveKey(), fileId, storageCallback);
}

void StorageQueryHandler::closeFile(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to close file {}",
                                        m_callId, fileId);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::close_file_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->closeFile(m_contractEnvironment.driveKey(), fileId, storageCallback);
}

void StorageQueryHandler::createFSIterator(const std::string& path, bool recursive,
                                                std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to create an iterator at {}",
                                        m_callId, path);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[asyncQuery, storageCallback] = createAsyncQuery<uint64_t>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(
                tl::make_unexpected(storage::make_error_code(storage::StorageError::create_iterator_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->directoryIteratorCreate(m_contractEnvironment.driveKey(), getPrefixedPath(path), recursive, storageCallback);
}

void
StorageQueryHandler::hasNextIterator(uint64_t iteratorID, std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info(
            "Contract call {} requested to query whether there is any more entries in iterator {}",
            m_callId, iteratorID);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[asyncQuery, storageCallback] = createAsyncQuery<bool>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(
                tl::make_unexpected(storage::make_error_code(storage::StorageError::iterator_has_next_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->directoryIteratorHasNext(m_contractEnvironment.driveKey(), iteratorID, storageCallback);
}

void StorageQueryHandler::nextIterator(uint64_t iteratorID,
                                            std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to the next entry in iterator {}",
                                        m_callId, iteratorID);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[asyncQuery, storageCallback] = createAsyncQuery<std::string>([=, this](auto&& res) {
        m_asyncQuery.reset();
        if (!res) {
            callback->postReply(
                    tl::make_unexpected(storage::make_error_code(storage::StorageError::iterator_next_error)));
        } else {
            std::vector<uint8_t> reply(res->begin(), res->end());
            callback->postReply(std::move(reply));
        }
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::iterator_next_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->directoryIteratorNext(m_contractEnvironment.driveKey(), iteratorID, storageCallback);
}

void
StorageQueryHandler::removeFileIterator(uint64_t iteratorID, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to remove the file pointed by iterator {}",
                                        m_callId, iteratorID);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    // auto [asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) { callback->postReply(res); },
    //                                                             [=] {
    //                                                                 callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::iterator_remove_error)));
    //                                                             },
    //                                                             m_executorEnvironment, true, true);

    // storage->directoryIteratorRemove(m_contractEnvironment.driveKey(), iteratorID, storageCallback);
    callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::iterator_remove_error)));
}

void
StorageQueryHandler::destroyFSIterator(uint64_t iteratorID, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to destroy iterator {}",
                                        m_callId, iteratorID);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(
                tl::make_unexpected(storage::make_error_code(storage::StorageError::destroy_iterator_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->directoryIteratorDestroy(m_contractEnvironment.driveKey(), iteratorID, storageCallback);
}

void StorageQueryHandler::pathExist(const std::string& path, std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to query whether path {} exists",
                                        m_callId, path);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[asyncQuery, storageCallback] = createAsyncQuery<bool>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::path_exist_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->pathExist(m_contractEnvironment.driveKey(), getPrefixedPath(path), storageCallback);
}

void StorageQueryHandler::isFile(const std::string& path, std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to query whether entry {} is a file",
                                        m_callId, path);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[asyncQuery, storageCallback] = createAsyncQuery<bool>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::is_file_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->isFile(m_contractEnvironment.driveKey(), getPrefixedPath(path), storageCallback);
}

void StorageQueryHandler::createDir(const std::string& path, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to create a directory {}",
                                        m_callId, path);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(
                tl::make_unexpected(storage::make_error_code(storage::StorageError::create_directory_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->createDirectories(m_contractEnvironment.driveKey(), getPrefixedPath(path), storageCallback);
}

void StorageQueryHandler::moveFile(const std::string& oldPath, const std::string& newPath,
                                        std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to move file {} to {}",
                                        m_callId, oldPath, newPath);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::move_file_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->moveFilesystemEntry(m_contractEnvironment.driveKey(), getPrefixedPath(oldPath), getPrefixedPath(newPath), storageCallback);
}

void
StorageQueryHandler::removeFsEntry(const std::string& path, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to remove file {}",
                                        m_callId, path);

    auto storage = m_executorEnvironment.storage().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto[asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::remove_file_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->removeFilesystemEntry(m_contractEnvironment.driveKey(), getPrefixedPath(path), storageCallback);
}

std::string StorageQueryHandler::getPrefixedPath(const std::string& path) {
    if (m_pathPrefix.empty()) {
        return path;
    }

    auto prefixedPath = m_pathPrefix + "/" + path;
    if (prefixedPath.ends_with('/')) {
        prefixedPath.pop_back();
    }

    return prefixedPath;
}

} // namespace sirius::contract