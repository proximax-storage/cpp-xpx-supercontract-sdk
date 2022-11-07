/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "TestUtils.h"
#include "storage/StorageErrorCode.h"
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
    int m_expectedSize;

public:
    FilesystemSimpleTraversal(T&& fileHandler, int expectedSize)
        : m_fileHandler(std::move(fileHandler)), m_expectedSize(expectedSize) {}

    void acceptFolder(const Folder& folder) override {
        ASSERT_FALSE(rootFolderVisited);
        rootFolderVisited = true;

        const auto& children = folder.children();

        ASSERT_EQ(children.size(), m_expectedSize);

        for (const auto& [name, entry] : children) {
            entry->acceptTraversal(*this);
        }
    }

    void acceptFile(const File& file) override {
        ASSERT_EQ(file.name(), "test.txt");
        m_fileHandler(file.name());
    }
};

void onFilesystemReceived(const DriveKey& driveKey,
                          GlobalEnvironment& environment,
                          std::shared_ptr<Storage> pStorage,
                          std::promise<void>& promise,
                          std::unique_ptr<Folder> folder) {
    FilesystemSimpleTraversal traversal([=, &environment, &promise](const std::string& path) {}, 0);
    traversal.acceptFolder(*folder);
    promise.set_value();
}

void onAppliedStorageModifications(const DriveKey& driveKey,
                                   GlobalEnvironment& environment,
                                   std::shared_ptr<Storage> pStorage,
                                   std::promise<void>& promise) {

    auto [_, callback] = createAsyncQuery<std::unique_ptr<Folder>>([=, &environment, &promise](auto&& res) {
        ASSERT_TRUE(res);
        onFilesystemReceived(driveKey, environment, pStorage, promise, std::move(*res)); }, [] {}, environment, false, true);
    pStorage->filesystem(driveKey, callback);
}

void onEvaluatedStorageHash(const DriveKey& driveKey,
                            GlobalEnvironment& environment,
                            std::shared_ptr<Storage> pStorage,
                            std::promise<void>& barrier,
                            const StorageState& digest) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onAppliedStorageModifications(driveKey, environment, pStorage, barrier); }, [] {}, environment, false, true);
    pStorage->applyStorageModifications(driveKey, true, callback);
}

void onAppliedSandboxModifications(const DriveKey& driveKey,
                                   GlobalEnvironment& environment,
                                   std::shared_ptr<Storage> pStorage,
                                   std::promise<void>& barrier,
                                   const SandboxModificationDigest& digest) {
    auto [_, callback] = createAsyncQuery<StorageState>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onEvaluatedStorageHash(driveKey, environment, pStorage, barrier, *res); }, [] {}, environment, false, true);

    pStorage->evaluateStorageHash(driveKey, callback);
}

void onFileRemoved(const DriveKey& driveKey,
                   GlobalEnvironment& environment,
                   std::shared_ptr<Storage> pStorage,
                   std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<SandboxModificationDigest>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onAppliedSandboxModifications(driveKey, environment, pStorage, barrier, *res); }, [] {}, environment, false, true);

    pStorage->applySandboxStorageModifications(driveKey, true, callback);
}

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     std::shared_ptr<Storage> pStorage,
                                     std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT_EQ(res.error(), StorageError::remove_file_error);
        onFileRemoved(driveKey, environment, pStorage, barrier); }, [] {}, environment, false, true);

    pStorage->removeFilesystemEntry(driveKey, "test.txt", callback);
}

void onModificationsInitiated(const DriveKey& driveKey,
                              GlobalEnvironment& environment,
                              std::shared_ptr<Storage> pStorage,
                              std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onSandboxModificationsInitiated(driveKey, environment, pStorage, barrier); }, [] {}, environment, false, true);

    pStorage->initiateSandboxModifications(driveKey, callback);
}

TEST(Storage, RemoveNonExisting) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    DriveKey driveKey{{16}};

    std::promise<void> pRemove;
    auto barrierRemove = pRemove.get_future();

    threadManager.execute([&] {
        std::string address = "127.0.0.1:5551";

        auto pStorage = std::make_shared<RPCStorage>(environment, address);

        auto [_, callback] = createAsyncQuery<void>([=, &environment, &pRemove](auto&& res) {
            ASSERT_TRUE(res);
            onModificationsInitiated(driveKey, environment, pStorage, pRemove); }, [] {}, environment, false, true);
        pStorage->initiateModifications(driveKey, callback);
    });

    barrierRemove.get();

    threadManager.stop();
}
}
} // namespace sirius::contract::storage::test