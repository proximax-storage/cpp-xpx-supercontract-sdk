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

namespace createFolders {
static const std::string folders[6] = {"test", "drive", "mod", "sc", "mod/gs", "drive/unit"};
static int i = 0;

void onFilesystemReceived(const DriveKey& driveKey,
                          GlobalEnvironment& environment,
                          ContextHolder& contextHolder,
                          std::promise<void>& promise,
                          std::unique_ptr<Folder> folder) {
    FilesystemSimpleTraversal traversal([=, &environment, &contextHolder, &promise](const std::string& path) {},
                                        1, environment);
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
    contextHolder.m_storageModification->applyStorageModifications(true, callback);
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

void onCreatedDirectory(const DriveKey& driveKey,
                        GlobalEnvironment& environment,
                        ContextHolder& contextHolder,
                        std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<SandboxModificationDigest>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onAppliedSandboxModifications(driveKey, environment, contextHolder, barrier, *res); }, [] {}, environment, false, true);

    auto [_1, createDirCallback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onCreatedDirectory(driveKey, environment, contextHolder, barrier); }, [] {}, environment, false, true);

    if (i == 6) {
        contextHolder.m_sandboxModification->applySandboxModifications(true, callback);
    } else {
        contextHolder.m_sandboxModification->createDirectories(createFolders::folders[i++], createDirCallback);
    }
}

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     ContextHolder& contextHolder,
                                     std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onCreatedDirectory(driveKey, environment, contextHolder, barrier); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->createDirectories(createFolders::folders[i++], callback);
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
} // namespace createFolders

namespace createFiles {
void onFileOpened(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  ContextHolder& contextHolder,
                  std::promise<void>& barrier, uint64_t fileId);
static const std::string files[11] = {"test/test.txt", "test/test2.txt", "drive/test.txt", "drive/test2.txt", "mod/test.txt", "mod/test2.txt", "mod/test3.txt", "sc/test.txt", "mod/gs/test.txt", "mod/gs/test2.txt", "drive/unit/test.txt"};
static int i = 0;

void onPathReceived(const DriveKey& driveKey,
                    GlobalEnvironment& environment,
                    ContextHolder& contextHolder,
                    std::promise<void>& promise,
                    const std::string& path) {
    ASSERT_TRUE(fs::exists(path));

    std::ifstream stream(path.c_str());

    std::ostringstream stringStream;
    stringStream << stream.rdbuf();

    std::string content = stringStream.str();

    ASSERT_EQ(content, "data");
}

void onFilesystemReceived(const DriveKey& driveKey,
                          GlobalEnvironment& environment,
                          ContextHolder& contextHolder,
                          std::promise<void>& promise,
                          std::unique_ptr<Folder> folder) {
    FilesystemSimpleTraversal traversal([=, &environment, &contextHolder, &promise](const std::string& path) {
        auto [_, callback] = createAsyncQuery<std::string>([=, &environment, &contextHolder, &promise](auto&& res) {
            ASSERT_TRUE(res);
            onPathReceived(driveKey, environment, contextHolder, promise, *res); }, [] {}, environment, false, true);
        contextHolder.m_storage->absolutePath(driveKey, createFiles::files[i++], callback);
    },
                                        2, environment);
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
    contextHolder.m_storageModification->applyStorageModifications(true, callback);
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

    auto [_1, openFileCallback] = createAsyncQuery<uint64_t>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onFileOpened(driveKey, environment, contextHolder, barrier, *res); }, [] {}, environment, false, true);

    if (i == 11) {
        i = 0;
        contextHolder.m_sandboxModification->applySandboxModifications(true, callback);
    } else {
        contextHolder.m_sandboxModification->openFile(createFiles::files[i++], OpenFileMode::WRITE, openFileCallback);
    }
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

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     ContextHolder& contextHolder,
                                     std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<uint64_t>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onFileOpened(driveKey, environment, contextHolder, barrier, *res); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->openFile(createFiles::files[i++], OpenFileMode::WRITE, callback);
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

namespace iterator {
static const std::string expected[17] = {"drive", "test.txt", "test2.txt", "unit", "test.txt", "mod", "gs", "test.txt", "test2.txt", "test.txt", "test2.txt", "test3.txt", "sc", "test.txt", "test", "test.txt", "test2.txt"};
static int i = 0;
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
    contextHolder.m_storageModification->applyStorageModifications(true, callback);
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
                         std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<SandboxModificationDigest>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onAppliedSandboxModifications(driveKey, environment, contextHolder, barrier, *res); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->applySandboxModifications(true, callback);
}

void onIteratorHasEnded(const DriveKey& driveKey,
                        GlobalEnvironment& environment,
                        ContextHolder& contextHolder,
                        std::promise<void>& barrier, uint64_t id) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_FALSE(res);
        ASSERT_EQ(res.error(), storage::StorageError::destroy_iterator_error);
        onIteratorDestroyed(driveKey, environment, contextHolder, barrier); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->directoryIteratorDestroy(1231643435, callback);
}

void onIteratorHasNext(const DriveKey& driveKey,
                       GlobalEnvironment& environment,
                       ContextHolder& contextHolder,
                       std::promise<void>& barrier, uint64_t id) {
    auto [_, callback] = createAsyncQuery<std::string>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        auto filename = *res;
        ASSERT_TRUE(filename == expected[i++]);
        onIteratorCreated(driveKey, environment, contextHolder, barrier, id); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->directoryIteratorNext(id, callback);
}

void onIteratorCreated(const DriveKey& driveKey,
                       GlobalEnvironment& environment,
                       ContextHolder& contextHolder,
                       std::promise<void>& barrier, uint64_t id) {
    auto [_, callback] = createAsyncQuery<bool>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        auto hasNext = *res;
        if (hasNext) {
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
        onSandboxModificationsInitiated(driveKey, environment, contextHolder, barrier); }, [] {}, environment, false, true);

    contextHolder.m_storageModification->initiateSandboxModification(callback);
}
} // namespace iterator

TEST(Storage, IteratorDestroyError) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    std::promise<void> p;
    auto barrier = p.get_future();

    ContextHolder contextHolder;
    
    DriveKey driveKey{{8}};

    threadManager.execute([&] {
        std::string address = "127.0.0.1:5551";

        contextHolder.m_storage = std::make_unique<RPCStorage>(environment, address);

        auto [_, callback] = createAsyncQuery<std::unique_ptr<StorageModification>>([=, &environment, &contextHolder, &p](auto&& res) {
            ASSERT_TRUE(res);
            contextHolder.m_storageModification = std::move(*res);
            createFolders::onModificationsInitiated(driveKey, environment, contextHolder, p); }, [] {}, environment, false, true);
        contextHolder.m_storage->initiateModifications(driveKey, utils::generateRandomByteValue<ModificationId>(), callback);
    });

    barrier.get();

    std::promise<void> pCreateFile;
    auto barrierCreateFile = pCreateFile.get_future();

    threadManager.execute([&] {
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
            iterator::onModificationsInitiated(driveKey, environment, contextHolder, pIterate); }, [] {}, environment, false, true);
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