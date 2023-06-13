/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
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

template<class T>
class FilesystemSimpleTraversal : public FilesystemTraversal {

private:
    bool rootFolderVisited = false;
    T m_fileHandler;
    bool m_before;

public:
    FilesystemSimpleTraversal(T&& fileHandler, bool before)
            : m_fileHandler(std::move(fileHandler)), m_before(before) {}

    void acceptFolder(const Folder& folder) override {
        // ASSERT_TRUE(rootFolderVisited);
        // rootFolderVisited = true;

        const auto& children = folder.children();

        if (m_before) {
            ASSERT_EQ(children.size(), 1);
        } else {
            if (folder.name() == "moved") {
                ASSERT_EQ(children.size(), 1);
            } else if (folder.name() == "tests") {
                ASSERT_EQ(children.size(), 0);
            } else {
                ASSERT_EQ(children.size(), 2);
            }
        }

        for (const auto&[name, entry] : children) {
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
    // move::createDirAndMove::FilesystemSimpleTraversal traversal([=, &environment, &promise](const std::string& path) {
    //     auto [_, callback] = createAsyncQuery<std::string>([=, &environment, &promise](auto&& res) {
    //         ASSERT_TRUE(res);
    //         onPathReceived(driveKey, environment, contextHolder, promise, res->m_absolutePath); }, [] {}, environment, false, true);
    //     contextHolder->absolutePath(driveKey, "tests/test.txt", callback);
    // },
    //                                                             true);
    // traversal.acceptFolder(*folder);
    promise.set_value();
}

void onAppliedStorageModifications(const DriveKey& driveKey,
                                   GlobalEnvironment& environment,
                                   ContextHolder& contextHolder,
                                   std::promise<void>& promise) {

    auto[_, callback] = createAsyncQuery<std::unique_ptr<Folder>>(
            [=, &environment, &contextHolder, &promise](auto&& res) {
                ASSERT_TRUE(res);
                onFilesystemReceived(driveKey, environment, contextHolder, promise, std::move(*res));
            }, [] {}, environment, false, true);
    contextHolder.m_storage->filesystem(driveKey, callback);
}

void onEvaluatedStorageHash(const DriveKey& driveKey,
                            GlobalEnvironment& environment,
                            ContextHolder& contextHolder,
                            std::promise<void>& barrier,
                            const StorageState& digest) {
    auto[_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onAppliedStorageModifications(driveKey, environment, contextHolder, barrier);
    }, [] {}, environment, false, true);
    contextHolder.m_storageModification->applyStorageModification(true, callback);
}

void onAppliedSandboxModifications(const DriveKey& driveKey,
                                   GlobalEnvironment& environment,
                                   ContextHolder& contextHolder,
                                   std::promise<void>& barrier,
                                   const SandboxModificationDigest& digest) {
    auto[_, callback] = createAsyncQuery<StorageState>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onEvaluatedStorageHash(driveKey, environment, contextHolder, barrier, *res);
    }, [] {}, environment, false, true);

    contextHolder.m_storageModification->evaluateStorageHash(callback);
}

void onCreatedDirectory(const DriveKey& driveKey,
                        GlobalEnvironment& environment,
                        ContextHolder& contextHolder,
                        std::promise<void>& barrier) {
    auto[_, callback] = createAsyncQuery<SandboxModificationDigest>(
            [=, &environment, &contextHolder, &barrier](auto&& res) {
                ASSERT_TRUE(res);
                onAppliedSandboxModifications(driveKey, environment, contextHolder, barrier, *res);
            }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->applySandboxModification(true, callback);
}

void onFileClosed(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  ContextHolder& contextHolder,
                  std::promise<void>& barrier) {
    auto[_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_FALSE(res);
        ASSERT_EQ(res.error(), storage::StorageError::create_directory_error);
        onCreatedDirectory(driveKey, environment, contextHolder, barrier);
    }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->createDirectories("tests", callback);
}

void onFileOpened(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  ContextHolder& contextHolder,
                  std::promise<void>& barrier, uint64_t fileID) {
    auto[_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onFileClosed(driveKey, environment, contextHolder, barrier);
    }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->closeFile(fileID, callback);
}

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     ContextHolder& contextHolder,
                                     std::promise<void>& barrier) {
    auto[_, callback] = createAsyncQuery<uint64_t>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onFileOpened(driveKey, environment, contextHolder, barrier, *res);
    }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->openFile("tests", OpenFileMode::WRITE, callback);
}

void onModificationsInitiated(const DriveKey& driveKey,
                              GlobalEnvironment& environment,
                              ContextHolder& contextHolder,
                              std::promise<void>& barrier) {
    auto[_, callback] = createAsyncQuery<std::unique_ptr<SandboxModification>>(
            [=, &environment, &contextHolder, &barrier](auto&& res) {
                ASSERT_TRUE(res);
                contextHolder.m_sandboxModification = std::move(*res);
                onSandboxModificationsInitiated(driveKey, environment, contextHolder, barrier);
            }, [] {}, environment, false, true);

    contextHolder.m_storageModification->initiateSandboxModification({}, callback);
}

TEST(Storage, CreateDirSameNameWithFile) {

    auto storageAddressOpt = storageAddress();

    if (!storageAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironment environment(std::make_shared<logging::Logger>(getLoggerConfig(), "executor"));
    auto& threadManager = environment.threadManager();

    std::promise<void> p;
    auto barrier = p.get_future();

    ContextHolder contextHolder;

     auto driveKey = utils::generateRandomByteValue<DriveKey>();

    threadManager.execute([&] {
        contextHolder.m_storage = std::make_unique<RPCStorage>(environment, *storageAddressOpt);

        auto[_, callback] = createAsyncQuery<std::unique_ptr<StorageModification>>(
                [=, &environment, &contextHolder, &p](auto&& res) {
                    ASSERT_TRUE(res);
                    contextHolder.m_storageModification = std::move(*res);
                    onModificationsInitiated(driveKey, environment, contextHolder, p);
                }, [] {}, environment, false, true);
        contextHolder.m_storage->initiateModifications(driveKey, utils::generateRandomByteValue<ModificationId>(),
                                                       callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));;

    threadManager.execute([&] {
        contextHolder.m_sandboxModification.reset();
        contextHolder.m_storageModification.reset();
        contextHolder.m_storage.reset();
    });

    threadManager.stop();
}
}
} // namespace sirius::contract::storage::test