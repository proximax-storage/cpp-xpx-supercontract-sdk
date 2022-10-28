/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "TestUtils.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <storage/File.h>
#include <storage/FilesystemTraversal.h>
#include <storage/Folder.h>
#include <storage/RPCStorage.h>

namespace fs = std::filesystem;

namespace sirius::contract::storage::test {

namespace {

template <class T>
class FilesystemSimpleTraversal : public FilesystemTraversal {

private:
    bool rootFolderVisited = false;
    T m_fileHandler;

public:
    FilesystemSimpleTraversal(T&& fileHandler)
        : m_fileHandler(std::move(fileHandler)) {}

    void acceptFolder(const Folder& folder) override {
        ASSERT_FALSE(rootFolderVisited);
        rootFolderVisited = true;

        const auto& children = folder.children();

        ASSERT_EQ(children.size(), 0);

        for (const auto& [name, entry] : children) {
            entry->acceptTraversal(*this);
        }
    }

    void acceptFile(const File& file) override {
        ASSERT_EQ(file.name(), "test.txt");
        m_fileHandler(file.name());
    }
};

namespace read {

void onFilesystemReceived(const DriveKey& driveKey,
                          GlobalEnvironment& environment,
                          std::shared_ptr<Storage> pStorage,
                          std::promise<void>& promise,
                          std::unique_ptr<Folder> folder) {
    FilesystemSimpleTraversal traversal([=, &environment, &promise](const std::string& path) {
        auto [_, callback] = createAsyncQuery<std::string>([=, &environment, &promise](auto&& res) { ASSERT(res, environment.logger()) }, [] {}, environment, false, true);
        pStorage->absolutePath(driveKey, "test.txt", callback);
    });
    traversal.acceptFolder(*folder);
    promise.set_value();
}

void onAppliedStorageModifications(const DriveKey& driveKey,
                                   GlobalEnvironment& environment,
                                   std::shared_ptr<Storage> pStorage,
                                   std::promise<void>& promise) {

    auto [_, callback] = createAsyncQuery<std::unique_ptr<Folder>>([=, &environment, &promise](auto&& res) {
        ASSERT(res, environment.logger())
        onFilesystemReceived(driveKey, environment, pStorage, promise, std::move(*res)); }, [] {}, environment, false, true);
    pStorage->filesystem(driveKey, callback);
}

void onEvaluatedStorageHash(const DriveKey& driveKey,
                            GlobalEnvironment& environment,
                            std::shared_ptr<Storage> pStorage,
                            std::promise<void>& barrier,
                            const StorageState& digest) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT(res, environment.logger())
        onAppliedStorageModifications(driveKey, environment, pStorage, barrier); }, [] {}, environment, false, true);
    pStorage->applyStorageModifications(driveKey, true, callback);
}

void onAppliedSandboxModifications(const DriveKey& driveKey,
                                   GlobalEnvironment& environment,
                                   std::shared_ptr<Storage> pStorage,
                                   std::promise<void>& barrier,
                                   const SandboxModificationDigest& digest) {
    auto [_, callback] = createAsyncQuery<StorageState>([=, &environment, &barrier](auto&& res) {
        ASSERT(res, environment.logger())
        onEvaluatedStorageHash(driveKey, environment, pStorage, barrier, *res); }, [] {}, environment, false, true);

    pStorage->evaluateStorageHash(driveKey, callback);
}

void onClosedFile(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  std::shared_ptr<Storage> pStorage,
                  std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<SandboxModificationDigest>([=, &environment, &barrier](auto&& res) {
        ASSERT(res, environment.logger())
        onAppliedSandboxModifications(driveKey, environment, pStorage, barrier, *res); }, [] {}, environment, false, true);

    pStorage->applySandboxStorageModifications(driveKey, true, callback);
}

void onReadFile(const DriveKey& driveKey,
                GlobalEnvironment& environment,
                std::shared_ptr<Storage> pStorage,
                std::promise<void>& barrier,
                uint64_t fileId) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT(res, environment.logger())
        onClosedFile(driveKey, environment, pStorage, barrier); }, [] {}, environment, false, true);

    pStorage->closeFile(driveKey, fileId, callback);
}

void onOpenedFile(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  std::shared_ptr<Storage> pStorage,
                  std::promise<void>& barrier,
                  uint64_t fileId) {
    auto [_, callback] = createAsyncQuery<std::vector<uint8_t>>([=, &environment, &barrier](auto&& res) {
        ASSERT(res, environment.logger())
        std::string actual(res->begin(), res->end());
        ASSERT_EQ(actual, "data");
        onReadFile(driveKey, environment, pStorage, barrier, fileId); }, [] {}, environment, false, true);

    pStorage->readFile(driveKey, fileId, 16 * 1024, callback);
}

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     std::shared_ptr<Storage> pStorage,
                                     std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<uint64_t>([=, &environment, &barrier](auto&& res) {
        ASSERT_EQ(res.error(), std::errc::io_error);
        onClosedFile(driveKey, environment, pStorage, barrier); }, [] {}, environment, false, true);

    pStorage->openFile(driveKey, "test.txt", OpenFileMode::READ, callback);
}

void onModificationsInitiated(const DriveKey& driveKey,
                              GlobalEnvironment& environment,
                              std::shared_ptr<Storage> pStorage,
                              std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT(res, environment.logger())
        onSandboxModificationsInitiated(driveKey, environment, pStorage, barrier); }, [] {}, environment, false, true);

    pStorage->initiateSandboxModifications(driveKey, callback);
}
} // namespace read

TEST(Storage, ReadNonExisting) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    DriveKey driveKey{{8}};

    std::promise<void> pRead;
    auto barrierRead = pRead.get_future();

    threadManager.execute([&] {
        std::string address = "127.0.0.1:5551";

        auto pStorage = std::make_shared<RPCStorage>(environment, address);

        auto [_, callback] = createAsyncQuery<void>([=, &environment, &pRead](auto&& res) {
            ASSERT(res, environment.logger())
            read::onModificationsInitiated(driveKey, environment, pStorage, pRead); }, [] {}, environment, false, true);
        pStorage->initiateModifications(driveKey, callback);
    });

    barrierRead.get();

    threadManager.stop();
}
}
} // namespace sirius::contract::storage::test