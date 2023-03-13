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
#include <RPCStorage.h>
#include <utils/Random.h>

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
                          ContextHolder& contextHolder,
                          std::promise<void>& promise,
                          std::unique_ptr<Folder> folder) {
    FilesystemSimpleTraversal traversal([=, &environment, &promise](const std::string& path) {}, 0);
    traversal.acceptFolder(*folder);
    promise.set_value();
}

void onAppliedStorageModifications(const DriveKey& driveKey,
                                   GlobalEnvironment& environment,
                                   ContextHolder& contextHolder,
                                   std::promise<void>& promise) {

    auto [_, callback] = createAsyncQuery<std::unique_ptr<Folder>>([=, &environment, &contextHolder, &promise](auto&& res) {
        ASSERT_TRUE(res);
        onFilesystemReceived(driveKey, environment, contextHolder, promise, std::move(*res)); }, [] {}, environment, false, true);
    contextHolder.m_storage->filesystem(driveKey, callback);
}

void onEvaluatedStorageHash(const DriveKey& driveKey,
                            GlobalEnvironment& environment,
                            ContextHolder& contextHolder,
                            std::promise<void>& barrier,
                            const StorageState& digest) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onAppliedStorageModifications(driveKey, environment, contextHolder, barrier); }, [] {}, environment, false, true);
    contextHolder.m_storageModification->applyStorageModification(true, callback);
}

void onAppliedSandboxModifications(const DriveKey& driveKey,
                                   GlobalEnvironment& environment,
                                   ContextHolder& contextHolder,
                                   std::promise<void>& barrier,
                                   const SandboxModificationDigest& digest) {
    auto [_, callback] = createAsyncQuery<StorageState>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onEvaluatedStorageHash(driveKey, environment, contextHolder, barrier, *res); }, [] {}, environment, false, true);

    contextHolder.m_storageModification->evaluateStorageHash(callback);
}

void onFileRemoved(const DriveKey& driveKey,
                   GlobalEnvironment& environment,
                   ContextHolder& contextHolder,
                   std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<SandboxModificationDigest>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onAppliedSandboxModifications(driveKey, environment, contextHolder, barrier, *res); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->applySandboxModification(true, callback);
}

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     ContextHolder& contextHolder,
                                     std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_EQ(res.error(), StorageError::remove_file_error);
        onFileRemoved(driveKey, environment, contextHolder, barrier); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->removeFilesystemEntry("test.txt", callback);
}

void onModificationsInitiated(const DriveKey& driveKey,
                              GlobalEnvironment& environment,
                              ContextHolder& contextHolder,
                              std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<std::unique_ptr<SandboxModification>>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        contextHolder.m_sandboxModification = std::move(*res);
        onSandboxModificationsInitiated(driveKey, environment, contextHolder, barrier); }, [] {}, environment, false, true);

    contextHolder.m_storageModification->initiateSandboxModification(callback);
}

TEST(Storage, RemoveNonExisting) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    ContextHolder contextHolder;

    DriveKey driveKey{{16}};

    std::promise<void> pRemove;
    auto barrierRemove = pRemove.get_future();

    threadManager.execute([&] {
        std::string address = "127.0.0.1:5551";

        contextHolder.m_storage = std::make_unique<RPCStorage>(environment, address);

        auto [_, callback] = createAsyncQuery<std::unique_ptr<StorageModification>>([=, &environment, &contextHolder, &pRemove](auto&& res) {
            ASSERT_TRUE(res);
            contextHolder.m_storageModification = std::move(*res);
            onModificationsInitiated(driveKey, environment, contextHolder, pRemove); }, [] {}, environment, false, true);
        contextHolder.m_storage->initiateModifications(driveKey, utils::generateRandomByteValue<ModificationId>(), callback);
    });

    barrierRemove.get();

    threadManager.execute([&] {
        contextHolder.m_sandboxModification.reset();
        contextHolder.m_storageModification.reset();
        contextHolder.m_storage.reset();
    });

    threadManager.stop();
}
}
} // namespace sirius::contract::storage::test