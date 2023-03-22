/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "SandboxModificationMock.h"
#include "gtest/gtest.h"

namespace sirius::contract::test {

SandboxModificationMock::SandboxModificationMock(std::shared_ptr<StorageInfo> info)
: m_info(std::move(info)) {}

void SandboxModificationMock::applySandboxModification(bool success,
                                                        std::shared_ptr<AsyncQueryCallback<storage::SandboxModificationDigest>> callback) {
    ASSERT_TRUE(m_info->m_actualBatch);
    if (success) {
        m_info->m_actualBatch->m_lowerSandboxState = nextState(m_info->m_actualBatch->m_lowerSandboxState);
    }
    m_info->m_actualBatch->m_calls.back() = success;
    callback->postReply(storage::SandboxModificationDigest{success, 0, 0});
}

void SandboxModificationMock::openFile(const std::string& path, storage::OpenFileMode mode,
                                       std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {

}

void SandboxModificationMock::writeFile(uint64_t fileId, const std::vector<uint8_t>& buffer,
                                        std::shared_ptr<AsyncQueryCallback<void>> callback) {

}

void SandboxModificationMock::readFile(uint64_t fileId, uint64_t bytesToRead,
                                       std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {

}

void SandboxModificationMock::closeFile(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {

}

void SandboxModificationMock::flush(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) {

}

void SandboxModificationMock::createDirectories(const std::string& path,
                                                std::shared_ptr<AsyncQueryCallback<void>> callback) {

}

void SandboxModificationMock::directoryIteratorCreate(const std::string& path, bool recursive,
                                                      std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {

}

void
SandboxModificationMock::directoryIteratorHasNext(uint64_t id, std::shared_ptr<AsyncQueryCallback<bool>> callback) {

}

void
SandboxModificationMock::directoryIteratorNext(uint64_t id, std::shared_ptr<AsyncQueryCallback<storage::DirectoryIteratorInfo>> callback) {

}

void
SandboxModificationMock::directoryIteratorDestroy(uint64_t id, std::shared_ptr<AsyncQueryCallback<void>> callback) {

}

void SandboxModificationMock::removeFilesystemEntry(const std::string& path,
                                                    std::shared_ptr<AsyncQueryCallback<void>> callback) {

}

void SandboxModificationMock::moveFilesystemEntry(const std::string& src, const std::string& dst,
                                                  std::shared_ptr<AsyncQueryCallback<void>> callback) {

}

void SandboxModificationMock::pathExist(const std::string& path, std::shared_ptr<AsyncQueryCallback<bool>> callback) {

}

void SandboxModificationMock::isFile(const std::string& path, std::shared_ptr<AsyncQueryCallback<bool>> callback) {

}

void
SandboxModificationMock::fileSize(const std::string& path, std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {

}


}


