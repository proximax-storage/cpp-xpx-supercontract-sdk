/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "CallExecutionEnvironment.h"
#include "internet/InternetUtils.h"
#include "storage/StorageErrorCode.h"
#include <internet/InternetErrorCode.h>

namespace sirius::contract {

CallExecutionEnvironment::CallExecutionEnvironment(const vm::CallRequest& request,
                                                   ExecutorEnvironment& executorEnvironment,
                                                   ContractEnvironment& contractEnvironment)
    : m_callRequest(request), m_executorEnvironment(executorEnvironment), m_contractEnvironment(
                                                                              contractEnvironment) {}

void CallExecutionEnvironment::openConnection(const std::string& url,
                                              std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto urlDescription = parseURL(url);

    if (!urlDescription) {
        m_executorEnvironment.logger().warn("Invalid URL \"{}\" at Contract Call {}", url, m_callRequest.m_callId);
        callback->postReply(
            tl::unexpected<std::error_code>(internet::make_error_code(internet::InternetError::invalid_url_error)));
        return;
    }

    auto connectionId = totalConnectionsCreated;
    totalConnectionsCreated++;

    m_executorEnvironment.logger().info("Contract call {} requested to open connection to {}, connection id: {}",
                                        m_callRequest.m_callId, url, connectionId);

    auto [query, connectionCallback] = createAsyncQuery<internet::InternetConnection>(
        [this, callback, connectionId](auto&& connection) {
            // If the callback is executed, 'this' will always be alive
            if (!connection) {
                callback->postReply(tl::unexpected<std::error_code>(connection.error()));
            } else {
                ASSERT(!m_internetConnections.contains(connectionId), m_executorEnvironment.logger())
                m_internetConnections.emplace(connectionId, std::move(*connection));
                callback->postReply(connectionId);
            }
            m_asyncQuery.reset();
        },
        [callback] {
            callback->postReply(tl::unexpected<std::error_code>(
                make_error_code(internet::InternetError::internet_unavailable)));
        },
        m_executorEnvironment, true, false);

    m_asyncQuery = std::move(query);

    if (urlDescription->ssl) {
        internet::InternetConnection::buildHttpsInternetConnection(m_executorEnvironment.sslContext(),
                                                                   m_executorEnvironment,
                                                                   urlDescription->host,
                                                                   urlDescription->port,
                                                                   urlDescription->target,
                                                                   m_executorEnvironment.executorConfig().internetBufferSize(),
                                                                   m_executorEnvironment.executorConfig().internetConnectionTimeoutMilliseconds(),
                                                                   m_executorEnvironment.executorConfig().ocspQueryTimerMilliseconds(),
                                                                   m_executorEnvironment.executorConfig().ocspQueryMaxEfforts(),
                                                                   internet::RevocationVerificationMode::SOFT,
                                                                   connectionCallback);
    } else {
        internet::InternetConnection::buildHttpInternetConnection(m_executorEnvironment,
                                                                  urlDescription->host,
                                                                  urlDescription->port,
                                                                  urlDescription->target,
                                                                  m_executorEnvironment.executorConfig().internetBufferSize(),
                                                                  m_executorEnvironment.executorConfig().internetConnectionTimeoutMilliseconds(),
                                                                  connectionCallback);
    }
}

void CallExecutionEnvironment::read(uint64_t connectionId,
                                    std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto connectionIt = m_internetConnections.find(connectionId);
    if (connectionIt == m_internetConnections.end()) {
        callback->postReply(tl::unexpected<std::error_code>(
            internet::make_error_code(internet::InternetError::invalid_resource_error)));
        return;
    }

    m_executorEnvironment.logger().info("Contract call {} requested to read from internet connection {}",
                                        m_callRequest.m_callId, connectionId);

    auto [query, readCallback] = createAsyncQuery<std::vector<uint8_t>>(
        [this, callback](auto&& data) {
            // If the callback is executed, 'this' will always be alive
            callback->postReply(std::forward<decltype(data)>(data));
            m_asyncQuery.reset();
        },
        [callback] {
            callback->postReply(tl::unexpected<std::error_code>(
                make_error_code(internet::InternetError::internet_unavailable)));
        },
        m_executorEnvironment, true, false);
    m_asyncQuery = std::move(query);
    connectionIt->second.read(readCallback);
}

void CallExecutionEnvironment::closeConnection(uint64_t connectionId, std::shared_ptr<AsyncQueryCallback<void>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to close internet connection {}",
                                        m_callRequest.m_callId, connectionId);

    auto connectionIt = m_internetConnections.find(connectionId);
    if (connectionIt == m_internetConnections.end()) {
        callback->postReply(
            tl::make_unexpected(internet::make_error_code(internet::InternetError::invalid_resource_error)));
        return;
    }

    m_internetConnections.erase(connectionIt);
    callback->postReply(expected<void>());
}

void CallExecutionEnvironment::openFile(const std::string& path, const std::string& mode,
                                        std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to open a file: {}",
                                        m_callRequest.m_callId, path);

    auto storage = m_executorEnvironment.storageModifier().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto [asyncQuery, storageCallback] = createAsyncQuery<uint64_t>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res)); }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::open_file_error))); }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);

    storage::OpenFileMode fileMode;
    if (mode == "w") {
        fileMode = storage::OpenFileMode::WRITE;
    } else {
        fileMode = storage::OpenFileMode::READ;
    }
    storage->openFile(m_contractEnvironment.driveKey(), path, fileMode, storageCallback);
}

void CallExecutionEnvironment::writeFile(uint64_t fileId, const std::vector<uint8_t>& buffer,
                                         std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to write to file {}",
                                        m_callRequest.m_callId, fileId);

    auto storage = m_executorEnvironment.storageModifier().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto [asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        if (!res) {
            callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::write_file_error)));
        }
        callback->postReply(buffer.size()); }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::write_file_error))); }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->writeFile(m_contractEnvironment.driveKey(), fileId, {buffer.begin(), buffer.end()}, storageCallback);
}

void CallExecutionEnvironment::readFile(uint64_t fileId,
                                        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to read file {}",
                                        m_callRequest.m_callId, fileId);

    auto storage = m_executorEnvironment.storageModifier().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto [asyncQuery, storageCallback] = createAsyncQuery<std::vector<uint8_t>>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res)); }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::read_file_error))); }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->readFile(m_contractEnvironment.driveKey(), fileId, 16 * 1024, storageCallback);
}

void CallExecutionEnvironment::flush(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to flush a write buffer to file {}",
                                        m_callRequest.m_callId, fileId);

    auto storage = m_executorEnvironment.storageModifier().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto [asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res)); }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::flush_error))); }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->flush(m_contractEnvironment.driveKey(), fileId, storageCallback);
}

void CallExecutionEnvironment::closeFile(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to close file {}",
                                        m_callRequest.m_callId, fileId);

    auto storage = m_executorEnvironment.storageModifier().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto [asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res)); }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::close_file_error))); }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->closeFile(m_contractEnvironment.driveKey(), fileId, storageCallback);
}

void CallExecutionEnvironment::createFSIterator(const std::string& path, bool recursive,
                                                std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to create an iterator at {}",
                                        m_callRequest.m_callId, path);

    auto storage = m_executorEnvironment.storageModifier().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto [asyncQuery, storageCallback] = createAsyncQuery<uint64_t>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res)); }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::create_iterator_error))); }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->directoryIteratorCreate(m_contractEnvironment.driveKey(), path, recursive, storageCallback);
}

void CallExecutionEnvironment::hasNextIterator(uint64_t iteratorID, std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info(
        "Contract call {} requested to query whether there is any more entries in iterator {}",
        m_callRequest.m_callId, iteratorID);

    auto storage = m_executorEnvironment.storageModifier().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto [asyncQuery, storageCallback] = createAsyncQuery<bool>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res)); }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::iterator_has_next_error))); }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->directoryIteratorHasNext(m_contractEnvironment.driveKey(), iteratorID, storageCallback);
}

void CallExecutionEnvironment::nextIterator(uint64_t iteratorID,
                                            std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to the next entry in iterator {}",
                                        m_callRequest.m_callId, iteratorID);

    auto storage = m_executorEnvironment.storageModifier().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto [asyncQuery, storageCallback] = createAsyncQuery<std::string>([=, this](auto&& res) {
        m_asyncQuery.reset();
        if (!res) {
            callback->postReply(
                    tl::make_unexpected(storage::make_error_code(storage::StorageError::iterator_next_error)));
        } else {
            std::vector<uint8_t> reply(res->begin(), res->end());
            callback->postReply(std::move(reply));
        } }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::iterator_next_error))); }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->directoryIteratorNext(m_contractEnvironment.driveKey(), iteratorID, storageCallback);
}

void CallExecutionEnvironment::removeFileIterator(uint64_t iteratorID, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to remove the file pointed by iterator {}",
                                        m_callRequest.m_callId, iteratorID);

    auto storage = m_executorEnvironment.storageModifier().lock();
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

void CallExecutionEnvironment::destroyFSIterator(uint64_t iteratorID, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to destroy iterator {}",
                                        m_callRequest.m_callId, iteratorID);

    auto storage = m_executorEnvironment.storageModifier().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto [asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res)); }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::destroy_iterator_error))); }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->directoryIteratorDestroy(m_contractEnvironment.driveKey(), iteratorID, storageCallback);
}

void CallExecutionEnvironment::pathExist(const std::string& path, std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to query whether path {} exists",
                                        m_callRequest.m_callId, path);

    auto storage = m_executorEnvironment.storageModifier().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto [asyncQuery, storageCallback] = createAsyncQuery<bool>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res)); }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::path_exist_error))); }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->pathExist(m_contractEnvironment.driveKey(), path, storageCallback);
}

void CallExecutionEnvironment::isFile(const std::string& path, std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to query whether entry {} is a file",
                                        m_callRequest.m_callId, path);

    auto storage = m_executorEnvironment.storageModifier().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto [asyncQuery, storageCallback] = createAsyncQuery<bool>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res)); }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::is_file_error))); }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->isFile(m_contractEnvironment.driveKey(), path, storageCallback);
}

void CallExecutionEnvironment::createDir(const std::string& path, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to create a directory {}",
                                        m_callRequest.m_callId, path);

    auto storage = m_executorEnvironment.storageModifier().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto [asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res)); }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::create_directory_error))); }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->createDirectories(m_contractEnvironment.driveKey(), path, storageCallback);
}

void CallExecutionEnvironment::moveFile(const std::string& oldPath, const std::string& newPath,
                                        std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to move file {} to {}",
                                        m_callRequest.m_callId, oldPath, newPath);

    auto storage = m_executorEnvironment.storageModifier().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto [asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res)); }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::move_file_error))); }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->moveFilesystemEntry(m_contractEnvironment.driveKey(), oldPath, newPath, storageCallback);
}

void CallExecutionEnvironment::removeFsEntry(const std::string& path, std::shared_ptr<AsyncQueryCallback<void>> callback) {
    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to remove file {}",
                                        m_callRequest.m_callId, path);

    auto storage = m_executorEnvironment.storageModifier().lock();
    if (!storage) {
        callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::storage_unavailable)));
        return;
    }

    auto [asyncQuery, storageCallback] = createAsyncQuery<void>([=, this](auto&& res) {
        m_asyncQuery.reset();
        callback->postReply(std::forward<decltype(res)>(res)); }, [=] { callback->postReply(tl::make_unexpected(storage::make_error_code(storage::StorageError::remove_file_error))); }, m_executorEnvironment, true, true);

    m_asyncQuery = std::move(asyncQuery);
    storage->removeFilesystemEntry(m_contractEnvironment.driveKey(), path, storageCallback);
}

} // namespace sirius::contract