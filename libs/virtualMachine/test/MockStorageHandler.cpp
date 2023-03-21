#include "MockStorageHandler.h"
#include <storage/StorageErrorCode.h>

namespace sirius::contract::vm::test {

namespace {

std::string evaluateAbsolutePath(const std::string& relativePath) {
    return std::filesystem::temp_directory_path() / relativePath;
}

}

MockStorageHandler::MockStorageHandler() {
}

void MockStorageHandler::openFile(
    const std::string& path,
    const std::string& mode,
    std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
    auto absolutePath = evaluateAbsolutePath(path);
    if (mode == "w") {
        m_writer.open(absolutePath);
    } else {
        m_reader.open(absolutePath);
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
    auto absolutePath = evaluateAbsolutePath(path);
    m_iterator = std::filesystem::directory_iterator{absolutePath};
    m_recursive = recursive;
    callback->postReply(231546131);
}

void MockStorageHandler::hasNextIterator(
    uint64_t iteratorId,
    std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    if (iteratorId != 231546131) {
        callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::iterator_next_error)));
    } else {
        if (m_iterator == end(m_iterator)) {
            callback->postReply(false);
        } else {
            callback->postReply(true);
        }
    }
}

void MockStorageHandler::nextIterator(
    uint64_t iteratorId,
    std::shared_ptr<AsyncQueryCallback<storage::DirectoryIteratorInfo>> callback) {
    if (iteratorId != 231546131) {
        callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::iterator_next_error)));
    } else {
        auto name = m_iterator->path().filename();
        m_iterator++;
        callback->postReply(storage::DirectoryIteratorInfo{name, 0});
    }
}

void MockStorageHandler::removeFileIterator(
    uint64_t iteratorId,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {
    if (iteratorId != 231546131) {
        callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::remove_file_error)));
    } else {
        auto path = m_iterator->path();
        std::filesystem::remove(path);
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
        expected<void> res;
        callback->postReply(std::move(res));
    }
}

void MockStorageHandler::pathExist(
    const std::string& path,
    std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    auto ret = std::filesystem::exists(evaluateAbsolutePath(path));
    callback->postReply(std::move(ret));
}

void MockStorageHandler::isFile(
    const std::string& path,
    std::shared_ptr<AsyncQueryCallback<bool>> callback) {
    auto ret = std::filesystem::is_regular_file(evaluateAbsolutePath(path));
    callback->postReply(std::move(ret));
}

void MockStorageHandler::createDir(
    const std::string& path,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {
    auto absolutePath = evaluateAbsolutePath(path);
    if (!std::filesystem::is_directory(absolutePath) || !std::filesystem::exists(absolutePath)) {
        std::filesystem::create_directory(absolutePath);
        expected<void> res;
        callback->postReply(std::move(res));
        return;
    }
    callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::create_directory_error)));
}

void MockStorageHandler::moveFile(
    const std::string& oldPath,
    const std::string& newPath,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {
    auto absoluteOldPath = evaluateAbsolutePath(oldPath);
    auto absoluteNewPath = evaluateAbsolutePath(newPath);
    if (!std::filesystem::is_regular_file(absoluteNewPath) || !std::filesystem::exists(absoluteNewPath)) {
        std::filesystem::rename(absoluteOldPath, absoluteNewPath);
        expected<void> res;
        callback->postReply(std::move(res));
    }
    callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::move_file_error)));
}

void MockStorageHandler::removeFsEntry(
    const std::string& path,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {
    std::filesystem::remove_all(evaluateAbsolutePath(path));
    expected<void> res;
    callback->postReply(std::move(res));
}

} // namespace sirius::contract::vm::test