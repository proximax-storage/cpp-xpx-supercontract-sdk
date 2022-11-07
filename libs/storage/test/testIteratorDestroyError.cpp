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
                          std::shared_ptr<Storage> pStorage,
                          std::promise<void>& promise,
                          std::unique_ptr<Folder> folder) {
    FilesystemSimpleTraversal traversal([=, &environment, &promise](const std::string& path) {},
                                        1, environment);
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

void onCreatedDirectory(const DriveKey& driveKey,
                        GlobalEnvironment& environment,
                        std::shared_ptr<Storage> pStorage,
                        std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<SandboxModificationDigest>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onAppliedSandboxModifications(driveKey, environment, pStorage, barrier, *res); }, [] {}, environment, false, true);

    auto [__, createDirCallback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onCreatedDirectory(driveKey, environment, pStorage, barrier); }, [] {}, environment, false, true);

    if (i == 6) {
        pStorage->applySandboxStorageModifications(driveKey, true, callback);
    } else {
        pStorage->createDirectories(driveKey, createFolders::folders[i++], createDirCallback);
    }
}

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     std::shared_ptr<Storage> pStorage,
                                     std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onCreatedDirectory(driveKey, environment, pStorage, barrier); }, [] {}, environment, false, true);

    pStorage->createDirectories(driveKey, createFolders::folders[i++], callback);
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
} // namespace createFolders

namespace createFiles {
void onFileOpened(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  std::shared_ptr<Storage> pStorage,
                  std::promise<void>& barrier, uint64_t fileId);
static const std::string files[11] = {"test/test.txt", "test/test2.txt", "drive/test.txt", "drive/test2.txt", "mod/test.txt", "mod/test2.txt", "mod/test3.txt", "sc/test.txt", "mod/gs/test.txt", "mod/gs/test2.txt", "drive/unit/test.txt"};
static int i = 0;

void onPathReceived(const DriveKey& driveKey,
                    GlobalEnvironment& environment,
                    std::shared_ptr<Storage> pStorage,
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
                          std::shared_ptr<Storage> pStorage,
                          std::promise<void>& promise,
                          std::unique_ptr<Folder> folder) {
    FilesystemSimpleTraversal traversal([=, &environment, &promise](const std::string& path) {
        auto [_, callback] = createAsyncQuery<std::string>([=, &environment, &promise](auto&& res) {
            ASSERT_TRUE(res);
            onPathReceived(driveKey, environment, pStorage, promise, *res); }, [] {}, environment, false, true);
        pStorage->absolutePath(driveKey, createFiles::files[i++], callback);
    },
                                        2, environment);
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

void onClosedFile(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  std::shared_ptr<Storage> pStorage,
                  std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<SandboxModificationDigest>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onAppliedSandboxModifications(driveKey, environment, pStorage, barrier, *res); }, [] {}, environment, false, true);

    auto [__, openFileCallback] = createAsyncQuery<uint64_t>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onFileOpened(driveKey, environment, pStorage, barrier, *res); }, [] {}, environment, false, true);

    if (i == 11) {
        i = 0;
        pStorage->applySandboxStorageModifications(driveKey, true, callback);
    } else {
        pStorage->openFile(driveKey, createFiles::files[i++], OpenFileMode::WRITE, openFileCallback);
    }
}

void onFlushed(const DriveKey& driveKey,
               GlobalEnvironment& environment,
               std::shared_ptr<Storage> pStorage,
               std::promise<void>& barrier, uint64_t fileId) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onClosedFile(driveKey, environment, pStorage, barrier); }, [] {}, environment, false, true);

    pStorage->closeFile(driveKey, fileId, callback);
}

void onWrittenFile(const DriveKey& driveKey,
                   GlobalEnvironment& environment,
                   std::shared_ptr<Storage> pStorage,
                   std::promise<void>& barrier,
                   uint64_t fileId) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onFlushed(driveKey, environment, pStorage, barrier, fileId); }, [] {}, environment, false, true);

    pStorage->flush(driveKey, fileId, callback);
}

void onFileOpened(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  std::shared_ptr<Storage> pStorage,
                  std::promise<void>& barrier, uint64_t fileId) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onWrittenFile(driveKey, environment, pStorage, barrier, fileId); }, [] {}, environment, false, true);

    std::string s("data");
    pStorage->writeFile(driveKey, fileId, {s.begin(), s.end()}, callback);
}

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     std::shared_ptr<Storage> pStorage,
                                     std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<uint64_t>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onFileOpened(driveKey, environment, pStorage, barrier, *res); }, [] {}, environment, false, true);

    pStorage->openFile(driveKey, createFiles::files[i++], OpenFileMode::WRITE, callback);
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
} // namespace createFiles

namespace iterator {
static const std::string expected[17] = {"drive", "test.txt", "test2.txt", "unit", "test.txt", "mod", "gs", "test.txt", "test2.txt", "test.txt", "test2.txt", "test3.txt", "sc", "test.txt", "test", "test.txt", "test2.txt"};
static int i = 0;
void onIteratorCreated(const DriveKey& driveKey,
                       GlobalEnvironment& environment,
                       std::shared_ptr<Storage> pStorage,
                       std::promise<void>& barrier, uint64_t id);

void onAppliedStorageModifications(const DriveKey& driveKey,
                                   GlobalEnvironment& environment,
                                   std::shared_ptr<Storage> pStorage,
                                   std::promise<void>& promise) {
    promise.set_value();
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

void onIteratorDestroyed(const DriveKey& driveKey,
                         GlobalEnvironment& environment,
                         std::shared_ptr<Storage> pStorage,
                         std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<SandboxModificationDigest>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onAppliedSandboxModifications(driveKey, environment, pStorage, barrier, *res); }, [] {}, environment, false, true);

    pStorage->applySandboxStorageModifications(driveKey, true, callback);
}

void onIteratorHasEnded(const DriveKey& driveKey,
                        GlobalEnvironment& environment,
                        std::shared_ptr<Storage> pStorage,
                        std::promise<void>& barrier, uint64_t id) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT_FALSE(res);
        ASSERT_EQ(res.error(), storage::StorageError::destroy_iterator_error);
        onIteratorDestroyed(driveKey, environment, pStorage, barrier); }, [] {}, environment, false, true);

    pStorage->directoryIteratorDestroy(driveKey, 1231643435, callback);
}

void onIteratorHasNext(const DriveKey& driveKey,
                       GlobalEnvironment& environment,
                       std::shared_ptr<Storage> pStorage,
                       std::promise<void>& barrier, uint64_t id) {
    auto [_, callback] = createAsyncQuery<std::string>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        auto filename = *res;
        ASSERT_TRUE(filename == expected[i++]);
        onIteratorCreated(driveKey, environment, pStorage, barrier, id); }, [] {}, environment, false, true);

    pStorage->directoryIteratorNext(driveKey, id, callback);
}

void onIteratorCreated(const DriveKey& driveKey,
                       GlobalEnvironment& environment,
                       std::shared_ptr<Storage> pStorage,
                       std::promise<void>& barrier, uint64_t id) {
    auto [_, callback] = createAsyncQuery<bool>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        auto hasNext = *res;
        if (hasNext) {
            onIteratorHasNext(driveKey, environment, pStorage, barrier, id);
        } else {
            onIteratorHasEnded(driveKey, environment, pStorage, barrier, id);
        } }, [] {}, environment, false, true);

    pStorage->directoryIteratorHasNext(driveKey, id, callback);
}

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     std::shared_ptr<Storage> pStorage,
                                     std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<uint64_t>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onIteratorCreated(driveKey, environment, pStorage, barrier, *res); }, [] {}, environment, false, true);

    pStorage->directoryIteratorCreate(driveKey, "", true, callback);
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
} // namespace iterator

TEST(Storage, IteratorDestroyError) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    std::promise<void> p;
    auto barrier = p.get_future();

    DriveKey driveKey{{8}};

    threadManager.execute([&] {
        std::string address = "127.0.0.1:5551";

        auto pStorage = std::make_shared<RPCStorage>(environment, address);

        auto [_, callback] = createAsyncQuery<void>([=, &environment, &p](auto&& res) {
            ASSERT_TRUE(res);
            createFolders::onModificationsInitiated(driveKey, environment, pStorage, p); }, [] {}, environment, false, true);
        pStorage->initiateModifications(driveKey, callback);
    });

    barrier.get();

    std::promise<void> pCreateFile;
    auto barrierCreateFile = pCreateFile.get_future();

    threadManager.execute([&] {
        std::string address = "127.0.0.1:5551";

        auto pStorage = std::make_shared<RPCStorage>(environment, address);

        auto [_, callback] = createAsyncQuery<void>([=, &environment, &pCreateFile](auto&& res) {
            ASSERT_TRUE(res);
            createFiles::onModificationsInitiated(driveKey, environment, pStorage, pCreateFile); }, [] {}, environment, false, true);
        pStorage->initiateModifications(driveKey, callback);
    });

    barrierCreateFile.get();

    std::promise<void> pIterate;
    auto barrierIterate = pIterate.get_future();

    threadManager.execute([&] {
        std::string address = "127.0.0.1:5551";

        auto pStorage = std::make_shared<RPCStorage>(environment, address);

        auto [_, callback] = createAsyncQuery<void>([=, &environment, &pIterate](auto&& res) {
            ASSERT_TRUE(res);
            iterator::onModificationsInitiated(driveKey, environment, pStorage, pIterate); }, [] {}, environment, false, true);
        pStorage->initiateModifications(driveKey, callback);
    });

    barrierIterate.get();

    threadManager.stop();
}
}
} // namespace sirius::contract::storage::test