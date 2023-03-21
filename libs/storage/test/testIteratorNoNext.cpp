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
    int m_phase;
    GlobalEnvironment& m_environment;

public:
    FilesystemSimpleTraversal(T&& fileHandler, int phase, GlobalEnvironment& environment)
        : m_fileHandler(std::move(fileHandler)), m_phase(phase), m_environment(environment) {}

    void acceptFolder(const Folder& folder) override {
        // ASSERT_TRUE(rootFolderVisited);
        // rootFolderVisited = true;

        const auto& children = folder.children();

        if (m_phase == 1) {
            if (folder.name() == "/") {
                ASSERT_EQ(children.size(), 4);
            } else if (folder.name() == "mod" || folder.name() == "drive") {
                ASSERT_EQ(children.size(), 1);
            } else {
                ASSERT_EQ(children.size(), 0);
            }
        } else if (m_phase == 2) {
            if (folder.name() == "/") {
                ASSERT_EQ(children.size(), 4);
            } else if (folder.name() == "drive") {
                ASSERT_EQ(children.size(), 3);
            } else if (folder.name() == "mod") {
                ASSERT_EQ(children.size(), 4);
            } else if (folder.name() == "test" || folder.name() == "gs") {
                ASSERT_EQ(children.size(), 2);
            } else {
                ASSERT_EQ(children.size(), 1);
            }
        }

        for (const auto& [name, entry] : children) {
            entry->acceptTraversal(*this);
        }
    }

    void acceptFile(const File& file) override {
        ASSERT(file.name() == "test.txt" || "test2.txt" || "test3.txt", m_environment.logger());
        m_fileHandler(file.name());
    }
};

namespace createFiles {
void onFileOpened(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  ContextHolder& contextHolder,
                  std::promise<void>& barrier, uint64_t fileId);

void onFilesystemReceived(const DriveKey& driveKey,
                          GlobalEnvironment& environment,
                          ContextHolder& contextHolder,
                          std::promise<void>& promise,
                          std::unique_ptr<Folder> folder) {
    // iterator::fs::FilesystemSimpleTraversal traversal([=, &environment, &contextHolder, &promise](const std::string& path) {},
    //                                                   2, environment);
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

void onClosedFile(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  ContextHolder& contextHolder,
                  std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<SandboxModificationDigest>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onAppliedSandboxModifications(driveKey, environment, contextHolder, barrier, *res); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->applySandboxModification(true, callback);
}

void onFlushed(const DriveKey& driveKey,
               GlobalEnvironment& environment,
               ContextHolder& contextHolder,
               std::promise<void>& barrier, uint64_t fileId) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onClosedFile(driveKey, environment, contextHolder, barrier); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->closeFile(fileId, callback);
}

void onWrittenFile(const DriveKey& driveKey,
                   GlobalEnvironment& environment,
                   ContextHolder& contextHolder,
                   std::promise<void>& barrier,
                   uint64_t fileId) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onFlushed(driveKey, environment, contextHolder, barrier, fileId); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->flush(fileId, callback);
}

void onFileOpened(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  ContextHolder& contextHolder,
                  std::promise<void>& barrier, uint64_t fileId) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onWrittenFile(driveKey, environment, contextHolder, barrier, fileId); }, [] {}, environment, false, true);

    std::string s("data");
    contextHolder.m_sandboxModification->writeFile(fileId, {s.begin(), s.end()}, callback);
}

void onCreatedDirectory(const DriveKey& driveKey,
                        GlobalEnvironment& environment,
                        ContextHolder& contextHolder,
                        std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<uint64_t>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onFileOpened(driveKey, environment, contextHolder, barrier, *res); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->openFile("tests/test.txt", OpenFileMode::WRITE, callback);
}

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     ContextHolder& contextHolder,
                                     std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onCreatedDirectory(driveKey, environment, contextHolder, barrier); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->createDirectories("tests", callback);
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
} // namespace createFiles

namespace iteratorNoNext {
static bool flag = false;

void onIteratorCreated(const DriveKey& driveKey,
                       GlobalEnvironment& environment,
                       ContextHolder& contextHolder,
                       std::promise<void>& barrier, uint64_t id);

void onAppliedStorageModifications(const DriveKey& driveKey,
                                   GlobalEnvironment& environment,
                                   ContextHolder& contextHolder,
                                   std::promise<void>& promise) {
    promise.set_value();
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

void onIteratorDestroyed(const DriveKey& driveKey,
                         GlobalEnvironment& environment,
                         ContextHolder& contextHolder,
                         std::promise<void>& barrier, uint64_t id) {
    auto [_, callback] = createAsyncQuery<SandboxModificationDigest>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onAppliedSandboxModifications(driveKey, environment, contextHolder, barrier, *res); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->applySandboxModification(true, callback);
}

void onIteratorHasEnded(const DriveKey& driveKey,
                        GlobalEnvironment& environment,
                        ContextHolder& contextHolder,
                        std::promise<void>& barrier, uint64_t id) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onIteratorDestroyed(driveKey, environment, contextHolder, barrier, id); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->directoryIteratorDestroy(id, callback);
}

void onIteratorHasNext(const DriveKey& driveKey,
                       GlobalEnvironment& environment,
                       ContextHolder& contextHolder,
                       std::promise<void>& barrier, uint64_t id) {
    auto [_, callback] = createAsyncQuery<DirectoryIteratorInfo>([=, &environment, &contextHolder, &barrier](auto&& res) {
        if (!res) {
            ASSERT_EQ(res.error(), StorageError::iterator_next_error);
            flag = true;
        }
        onIteratorCreated(driveKey, environment, contextHolder, barrier, id); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->directoryIteratorNext(id, callback);
}

void onIteratorCreated(const DriveKey& driveKey,
                       GlobalEnvironment& environment,
                       ContextHolder& contextHolder,
                       std::promise<void>& barrier, uint64_t id) {
    auto [_, callback] = createAsyncQuery<bool>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        if (!flag) {
            onIteratorHasNext(driveKey, environment, contextHolder, barrier, id);
        } else {
            onIteratorHasEnded(driveKey, environment, contextHolder, barrier, id);
        } }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->directoryIteratorHasNext(id, callback);
}

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     ContextHolder& contextHolder,
                                     std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<uint64_t>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onIteratorCreated(driveKey, environment, contextHolder, barrier, *res); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->directoryIteratorCreate("", true, callback);
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
} // namespace iteratorNoNext

TEST(Storage, IteratorNoNext) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    ContextHolder contextHolder;

    DriveKey driveKey{{10}};

    std::promise<void> pCreateFile;
    auto barrierCreateFile = pCreateFile.get_future();

    threadManager.execute([&] {
        std::string address = "127.0.0.1:5551";

        contextHolder.m_storage = std::make_unique<RPCStorage>(environment, address);

        auto [_, callback] = createAsyncQuery<std::unique_ptr<StorageModification>>([=, &environment, &contextHolder, &pCreateFile](auto&& res) {
            ASSERT_TRUE(res);
            contextHolder.m_storageModification = std::move(*res);
            createFiles::onModificationsInitiated(driveKey, environment, contextHolder, pCreateFile); }, [] {}, environment, false, true);
        contextHolder.m_storage->initiateModifications(driveKey, utils::generateRandomByteValue<ModificationId>(), callback);
    });

    barrierCreateFile.get();

    std::promise<void> pIterate;
    auto barrierIterate = pIterate.get_future();

    threadManager.execute([&] {
        auto [_, callback] = createAsyncQuery<std::unique_ptr<StorageModification>>([=, &environment, &contextHolder, &pIterate](auto&& res) {
            ASSERT_TRUE(res);
            contextHolder.m_storageModification = std::move(*res);
            iteratorNoNext::onModificationsInitiated(driveKey, environment, contextHolder, pIterate); }, [] {}, environment, false, true);
        contextHolder.m_storage->initiateModifications(driveKey, utils::generateRandomByteValue<ModificationId>(), callback);
    });

    barrierIterate.get();

    threadManager.execute([&] {
        contextHolder.m_sandboxModification.reset();
        contextHolder.m_storageModification.reset();
        contextHolder.m_storage.reset();
    });

    threadManager.stop();
}
}
} // namespace sirius::contract::storage::test