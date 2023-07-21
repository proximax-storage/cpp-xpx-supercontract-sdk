/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "StorageQueryHandler.h"

namespace sirius::contract {

StorageQueryHandler::StorageQueryHandler(const CallId& callId,
                                         ExecutorEnvironment& executorEnvironment,
                                         ContractEnvironment& contractEnvironment,
                                         std::shared_ptr<storage::SandboxModification> sandboxModification,
                                         const std::string& pathPrefix)
                                         : m_callId(callId)
                                         , m_executorEnvironment(executorEnvironment)
                                         , m_contractEnvironment(contractEnvironment)
                                         , m_sandboxModification(std::move(sandboxModification))
                                         , m_pathPrefix(pathPrefix) {}

void StorageQueryHandler::openFile(const std::string& path, const std::string& mode,
                                        std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

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
    m_sandboxModification->openFile(getPrefixedPath(path), fileMode, storageCallback);
}

void StorageQueryHandler::writeFile(uint64_t fileId, const std::vector<uint8_t>& buffer,
                                         std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

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
    m_sandboxModification->writeFile(fileId, {buffer.begin(), buffer.end()}, storageCallback);
}

void StorageQueryHandler::readFile(uint64_t fileId,
                                        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto[asyncQuery, storageCallback] = createAsyncQuery<std::vector<uint8_t>>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::read_file_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    m_sandboxModification->readFile(fileId, 16 * 1024, storageCallback);
}

void StorageQueryHandler::flush(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto[asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
                                                                   m_asyncQuery.reset();
                                                                   callback->postReply(std::forward<decltype(res)>(res));
                                                               }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::flush_error))); },
                                                               m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    m_sandboxModification->flush(fileId, storageCallback);
}

void StorageQueryHandler::closeFile(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto[asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::close_file_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    m_sandboxModification->closeFile(fileId, storageCallback);
}

void StorageQueryHandler::createFSIterator(const std::string& path, bool recursive,
                                                std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto[asyncQuery, storageCallback] = createAsyncQuery<uint64_t>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(
                tl::make_unexpected(storage::make_error_code(storage::StorageError::create_iterator_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    m_sandboxModification->directoryIteratorCreate(getPrefixedPath(path), recursive, storageCallback);
}

void
StorageQueryHandler::hasNextIterator(uint64_t iteratorID, std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto[asyncQuery, storageCallback] = createAsyncQuery<bool>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(
                tl::make_unexpected(storage::make_error_code(storage::StorageError::iterator_has_next_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    m_sandboxModification->directoryIteratorHasNext(iteratorID, storageCallback);
}

void StorageQueryHandler::nextIterator(uint64_t iteratorID,
                                            std::shared_ptr<AsyncQueryCallback<storage::DirectoryIteratorInfo>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto[asyncQuery, storageCallback] = createAsyncQuery<storage::DirectoryIteratorInfo>([=, this](auto&& res) {
        m_asyncQuery.reset();
        if (!res) {
            callback->postReply(
                    tl::make_unexpected(storage::make_error_code(storage::StorageError::iterator_next_error)));
        } else {
            callback->postReply(std::move(*res));
        }
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::iterator_next_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    m_sandboxModification->directoryIteratorNext(iteratorID, storageCallback);
}

void
StorageQueryHandler::removeFileIterator(uint64_t iteratorID, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::iterator_remove_error)));
}

void
StorageQueryHandler::destroyFSIterator(uint64_t iteratorID, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto[asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(
                tl::make_unexpected(storage::make_error_code(storage::StorageError::destroy_iterator_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    m_sandboxModification->directoryIteratorDestroy(iteratorID, storageCallback);
}

void StorageQueryHandler::pathExist(const std::string& path, std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto[asyncQuery, storageCallback] = createAsyncQuery<bool>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::path_exist_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    m_sandboxModification->pathExist(getPrefixedPath(path), storageCallback);
}

void StorageQueryHandler::isFile(const std::string& path, std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto[asyncQuery, storageCallback] = createAsyncQuery<bool>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::is_file_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    m_sandboxModification->isFile(getPrefixedPath(path), storageCallback);
}

void StorageQueryHandler::createDir(const std::string& path, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto[asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(
                tl::make_unexpected(storage::make_error_code(storage::StorageError::create_directory_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    m_sandboxModification->createDirectories(getPrefixedPath(path), storageCallback);
}

void StorageQueryHandler::moveFile(const std::string& oldPath, const std::string& newPath,
                                        std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto[asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::move_file_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    m_sandboxModification->moveFilesystemEntry(getPrefixedPath(oldPath), getPrefixedPath(newPath), storageCallback);
}

void
StorageQueryHandler::removeFsEntry(const std::string& path, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto[asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
    }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::remove_file_error)));
    }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    m_sandboxModification->removeFilesystemEntry(getPrefixedPath(path), storageCallback);
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

void StorageQueryHandler::fileSize(const std::string& path, std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto[asyncQuery, storageCallback] = createAsyncQuery<uint64_t>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res));
        }, [=] {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::is_file_error)));
        }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    m_sandboxModification->fileSize(getPrefixedPath(path), storageCallback);
}

} // namespace sirius::contract