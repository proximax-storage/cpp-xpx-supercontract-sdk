#include "FaultyMockStorageHandler.h"
#include <storage/StorageErrorCode.h>

namespace sirius::contract::vm::test {
FaultyMockStorageHandler::FaultyMockStorageHandler() {
}

void FaultyMockStorageHandler::openFile(
    const std::string& path,
    const std::string& mode,
    std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {

    callback->postReply(tl::make_unexpected(make_error_code(sirius::contract::storage::StorageError::storage_unavailable)));
}

void FaultyMockStorageHandler::readFile(
    uint64_t fileId,
    std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {}

void FaultyMockStorageHandler::writeFile(
    uint64_t fileId,
    const std::vector<uint8_t>& buffer,
    std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {}

void FaultyMockStorageHandler::flush(
    uint64_t fileId,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {}

void FaultyMockStorageHandler::closeFile(
    uint64_t fileId,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {
    expected<void> res;
    callback->postReply(std::move(res));
}

void FaultyMockStorageHandler::createFSIterator(
    const std::string& path,
    bool recursive,
    std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {}

void FaultyMockStorageHandler::hasNextIterator(
    uint64_t iteratorID,
    std::shared_ptr<AsyncQueryCallback<bool>> callback) {}

void FaultyMockStorageHandler::nextIterator(
    uint64_t iteratorID,
    std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {}

void FaultyMockStorageHandler::removeFileIterator(
    uint64_t iteratorID,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {}

void FaultyMockStorageHandler::destroyFSIterator(
    uint64_t iteratorID,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {}

void FaultyMockStorageHandler::pathExist(
    const std::string& path,
    std::shared_ptr<AsyncQueryCallback<bool>> callback) {}

void FaultyMockStorageHandler::isFile(
    const std::string& path,
    std::shared_ptr<AsyncQueryCallback<bool>> callback) {}

void FaultyMockStorageHandler::createDir(
    const std::string& path,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {}

void FaultyMockStorageHandler::moveFile(
    const std::string& oldPath,
    const std::string& newPath,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {}

void FaultyMockStorageHandler::removeFsEntry(
    const std::string& path,
    std::shared_ptr<AsyncQueryCallback<void>> callback) {}

} // namespace sirius::contract::vm::test