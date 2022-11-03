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

namespace iterator::fs {
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
} // namespace iterator::fs

namespace iteratorNoNext::createFiles {
void onFileOpened(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  std::shared_ptr<Storage> pStorage,
                  std::promise<void>& barrier, uint64_t fileId);

void onFilesystemReceived(const DriveKey& driveKey,
                          GlobalEnvironment& environment,
                          std::shared_ptr<Storage> pStorage,
                          std::promise<void>& promise,
                          std::unique_ptr<Folder> folder) {
    // iterator::fs::FilesystemSimpleTraversal traversal([=, &environment, &promise](const std::string& path) {},
    //                                                   2, environment);
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

    pStorage->applySandboxStorageModifications(driveKey, true, callback);
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

void onCreatedDirectory(const DriveKey& driveKey,
                        GlobalEnvironment& environment,
                        std::shared_ptr<Storage> pStorage,
                        std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<uint64_t>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onFileOpened(driveKey, environment, pStorage, barrier, *res); }, [] {}, environment, false, true);

    pStorage->openFile(driveKey, "tests/test.txt", OpenFileMode::WRITE, callback);
}

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     std::shared_ptr<Storage> pStorage,
                                     std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onCreatedDirectory(driveKey, environment, pStorage, barrier); }, [] {}, environment, false, true);

    pStorage->createDirectories(driveKey, "tests", callback);
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
} // namespace iteratorNoNext::createFiles

namespace iteratorNoNext {
static bool flag = false;

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
                         std::promise<void>& barrier, uint64_t id) {
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
        ASSERT_TRUE(res);
        onIteratorDestroyed(driveKey, environment, pStorage, barrier, id); }, [] {}, environment, false, true);

    pStorage->directoryIteratorDestroy(driveKey, id, callback);
}

void onIteratorHasNext(const DriveKey& driveKey,
                       GlobalEnvironment& environment,
                       std::shared_ptr<Storage> pStorage,
                       std::promise<void>& barrier, uint64_t id) {
    auto [_, callback] = createAsyncQuery<std::string>([=, &environment, &barrier](auto&& res) {
        if (!res) {
            ASSERT_EQ(res.error(), StorageError::iterator_next_error);
            flag = true;
        }
        onIteratorCreated(driveKey, environment, pStorage, barrier, id); }, [] {}, environment, false, true);

    pStorage->directoryIteratorNext(driveKey, id, callback);
}

void onIteratorCreated(const DriveKey& driveKey,
                       GlobalEnvironment& environment,
                       std::shared_ptr<Storage> pStorage,
                       std::promise<void>& barrier, uint64_t id) {
    auto [_, callback] = createAsyncQuery<bool>([=, &environment, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        if (!flag) {
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
} // namespace iteratorNoNext

TEST(Storage, IteratorNoNext) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    DriveKey driveKey{{7}};

    std::promise<void> pCreateFile;
    auto barrierCreateFile = pCreateFile.get_future();

    threadManager.execute([&] {
        std::string address = "127.0.0.1:5551";

        auto pStorage = std::make_shared<RPCStorage>(environment, address);

        auto [_, callback] = createAsyncQuery<void>([=, &environment, &pCreateFile](auto&& res) {
            ASSERT_TRUE(res);
            iteratorNoNext::createFiles::onModificationsInitiated(driveKey, environment, pStorage, pCreateFile); }, [] {}, environment, false, true);
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
            iteratorNoNext::onModificationsInitiated(driveKey, environment, pStorage, pIterate); }, [] {}, environment, false, true);
        pStorage->initiateModifications(driveKey, callback);
    });

    barrierIterate.get();

    threadManager.stop();
}
} // namespace sirius::contract::storage::test