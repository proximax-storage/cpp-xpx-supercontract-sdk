#include "MockStorageHandler.h"
#include <storage/StorageErrorCode.h>

namespace sirius::contract::vm::test {
MockStorageHandler::MockStorageHandler() {
}

void MockStorageHandler::openFile(
    const std::string& path,
    const std::string& mode,
    std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    if (mode == "w") {
        m_writer.open(path);
    } else {
        m_reader.open(path);
    }
    callback->postReply(std::move(102112022));
}

void MockStorageHandler::readFile(
    uint64_t fileId,
    std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {
    if (fileId != 102112022) {
        callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::read_file_error)));
    } else {
        std::vector<uint8_t> buffer;
        std::string myText;
        int i = 0;
        while (getline(m_reader, myText) && i < 16 * 1024) {
            std::vector<uint8_t> temp(myText.begin(), myText.end());
            temp.push_back((int)'\n');
            buffer.insert(buffer.end(), temp.begin(), temp.end());
            i += myText.size();
        }
        callback->postReply(std::move(buffer));
    }
}

void MockStorageHandler::writeFile(
    uint64_t fileId,
    const std::vector<uint8_t>& buffer,
    std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    if (fileId != 102112022) {
        callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::write_file_error)));
    } else {
        std::string string(buffer.begin(), buffer.end());
        m_writer << string;
        callback->postReply(std::move(string.size()));
    }
}

void MockStorageHandler::flush(
    uint64_t fileId,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {
    if (fileId != 102112022) {
        callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::flush_error)));
    } else {
        m_writer.flush();
        expected<void> res;
        callback->postReply(std::move(res));
    }
}

void MockStorageHandler::closeFile(
    uint64_t fileId,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {
    if (fileId != 102112022) {
        callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::close_file_error)));
    } else {
        m_writer.close();
        m_reader.close();
        expected<void> res;
        callback->postReply(std::move(res));
    }
}

void MockStorageHandler::createFSIterator(
    const std::string& path,
    bool recursive,
    std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    m_iterator = std::filesystem::directory_iterator{path};
    m_recursive = recursive;
    callback->postReply(231546131);
}

void MockStorageHandler::hasNextIterator(
    uint64_t iteratorId,
    std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    if (iteratorId != 231546131) {
        callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::iterator_next_error)));
    } else {
        if (m_iterator->path().string() == "testFolder/internet") {
            callback->postReply(std::move(false));
        } else {
            m_iterator++;
            callback->postReply(std::move(true));
        }
    }
}

void MockStorageHandler::nextIterator(
    uint64_t iteratorId,
    std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {
    if (iteratorId != 231546131) {
        callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::iterator_next_error)));
    } else {
        auto ret = m_iterator->path().string();
        std::vector<uint8_t> name(ret.begin(), ret.end());
        callback->postReply(std::move(name));
    }
}

void MockStorageHandler::removeFileIterator(
    uint64_t iteratorId,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {
    if (iteratorId != 231546131) {
        callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::remove_file_error)));
    } else {
        auto path = m_iterator->path();
        auto ret = std::filesystem::remove(path);
        expected<void> res;
        callback->postReply(std::move(res));
    }
}

void MockStorageHandler::destroyFSIterator(
    uint64_t iteratorId,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {
    if (iteratorId != 231546131) {
        callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::destroy_iterator_error)));
    } else {
        m_iterator.~directory_iterator();
        expected<void> res;
        callback->postReply(std::move(res));
    }
}

void MockStorageHandler::pathExist(
    const std::string& path,
    std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    auto ret = std::filesystem::exists(path);
    callback->postReply(std::move(ret));
}

void MockStorageHandler::isFile(
    const std::string& path,
    std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    auto ret = std::filesystem::is_regular_file(path);
    callback->postReply(std::move(ret));
}

void MockStorageHandler::createDir(
    const std::string& path,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {
    if (!std::filesystem::is_directory(path) || !std::filesystem::exists(path)) {
        auto ret = std::filesystem::create_directory(path);
        expected<void> res;
        callback->postReply(std::move(res));
    }
    callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::create_directory_error)));
}

void MockStorageHandler::moveFile(
    const std::string& oldPath,
    const std::string& newPath,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {
    if (!std::filesystem::is_regular_file(newPath) || !std::filesystem::exists(newPath)) {
        std::filesystem::rename(oldPath, newPath);
        expected<void> res;
        callback->postReply(std::move(res));
    }
    callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::move_file_error)));
}

void MockStorageHandler::removeFile(
    const std::string& path,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {
    auto ret = std::filesystem::remove_all(path);
    expected<void> res;
    callback->postReply(std::move(res));
}

} // namespace sirius::contract::vm::test