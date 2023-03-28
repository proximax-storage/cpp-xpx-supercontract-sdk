/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
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

public:
    FilesystemSimpleTraversal(T&& fileHandler)
            : m_fileHandler(std::move(fileHandler)) {}

    void acceptFolder(const Folder& folder) override {
        ASSERT_FALSE(rootFolderVisited);
        rootFolderVisited = true;

        const auto& children = folder.children();

        ASSERT_EQ(children.size(), 0);

        for (const auto&[name, entry] : children) {
            entry->acceptTraversal(*this);
        }
    }

    void acceptFile(const File& file) override {
        ASSERT_EQ(file.name(), "test.txt");
        m_fileHandler(file.name());
    }
};

void onPathReceived(const DriveKey& driveKey,
                    GlobalEnvironment& environment,
                    ContextHolder& contextHolder,
                    std::promise<void>& promise,
                    const std::string& path) {
    ASSERT_FALSE(fs::exists(path));

    std::ifstream stream(path.c_str());

    std::ostringstream stringStream;
    stringStream << stream.rdbuf();

    std::string content = stringStream.str();

    ASSERT_EQ(content, "data");

    promise.set_value();
}

void onFilesystemReceived(const DriveKey& driveKey,
                          GlobalEnvironment& environment,
                          ContextHolder& contextHolder,
                          std::promise<void>& promise,
                          std::unique_ptr<Folder> folder) {
    FilesystemSimpleTraversal traversal([=, &environment, &contextHolder, &promise](const std::string& path) {
        auto[_, callback] = createAsyncQuery<FileInfo>([=, &environment, &contextHolder, &promise](auto&& res) {
            ASSERT_TRUE(res);
            onPathReceived(driveKey, environment, contextHolder, promise, res->m_absolutePath);
        }, [] {}, environment, false, true);
        contextHolder.m_storage->fileInfo(driveKey, "test.txt", callback);
    });
    traversal.acceptFolder(*folder);
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
    contextHolder.m_storageModification->applyStorageModification(false, callback);
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

void onClosedFile(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  ContextHolder& contextHolder,
                  std::promise<void>& barrier) {
    auto[_, callback] = createAsyncQuery<SandboxModificationDigest>(
            [=, &environment, &contextHolder, &barrier](auto&& res) {
                ASSERT_TRUE(res);
                auto r = *res;
                onAppliedSandboxModifications(driveKey, environment, contextHolder, barrier, r);
            }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->applySandboxModification(true, callback);
}

void onFlushed(const DriveKey& driveKey,
               GlobalEnvironment& environment,
               ContextHolder& contextHolder,
               std::promise<void>& barrier, uint64_t fileId) {
    auto[_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onClosedFile(driveKey, environment, contextHolder, barrier);
    }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->closeFile(fileId, callback);
}

void onWrittenFile(const DriveKey& driveKey,
                   GlobalEnvironment& environment,
                   ContextHolder& contextHolder,
                   std::promise<void>& barrier,
                   uint64_t fileId) {
    auto[_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onFlushed(driveKey, environment, contextHolder, barrier, fileId);
    }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->flush(fileId, callback);
}

void onOpenedFile(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  ContextHolder& contextHolder,
                  std::promise<void>& barrier,
                  uint64_t fileId) {
    auto[_, callback] = createAsyncQuery<void>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onWrittenFile(driveKey, environment, contextHolder, barrier, fileId);
    }, [] {}, environment, false, true);

    std::string s("data");
    contextHolder.m_sandboxModification->writeFile(fileId, {s.begin(), s.end()}, callback);
}

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     ContextHolder& contextHolder,
                                     std::promise<void>& barrier) {
    auto[_, callback] = createAsyncQuery<uint64_t>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onOpenedFile(driveKey, environment, contextHolder, barrier, *res);
    }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->openFile("test.txt", OpenFileMode::WRITE, callback);
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

    contextHolder.m_storageModification->initiateSandboxModification(callback);
}

TEST(Storage, DiscardStorageModification) {

    auto storageAddressOpt = storageAddress();

    if (!storageAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
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

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.execute([&] {
        contextHolder.m_sandboxModification.reset();
        contextHolder.m_storageModification.reset();
        contextHolder.m_storage.reset();
    });

    threadManager.stop();
}
}
} // namespace sirius::contract::storage::test