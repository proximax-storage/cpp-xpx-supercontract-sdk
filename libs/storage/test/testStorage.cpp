/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <gtest/gtest.h>
#include <storage/RPCStorage.h>
#include <storage/FilesystemTraversal.h>
#include <storage/File.h>
#include <storage/Folder.h>
#include "TestUtils.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace sirius::contract::storage::test {

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

        ASSERT_EQ(children.size(), 1);

        for (const auto&[name, entry]: children) {
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
                    std::shared_ptr<Storage> pStorage,
                    std::promise<void>& promise,
                    const std::string& path) {
    ASSERT_TRUE(fs::exists(path));

    std::ifstream stream(path.c_str());

    std::ostringstream stringStream;
    stringStream << stream.rdbuf();

    std::string content = stringStream.str();

    ASSERT_EQ(content, "data");

    promise.set_value();
}

void onFilesystemReceived(const DriveKey& driveKey,
                          GlobalEnvironment& environment,
                          std::shared_ptr<Storage> pStorage,
                          std::promise<void>& promise,
                          std::unique_ptr<Folder> folder) {
    FilesystemSimpleTraversal traversal([=, &environment, &promise](const std::string& path) {
        auto[_, callback] = createAsyncQuery<std::string>([=, &environment, &promise](auto&& res) {
            ASSERT(res, environment.logger())
            onPathReceived(driveKey, environment, pStorage, promise, *res);
        }, [] {}, environment, false, true);
        pStorage->absolutePath(driveKey, "test.txt", callback);
    });
    traversal.acceptFolder(*folder);
}

void onAppliedStorageModifications(const DriveKey& driveKey,
                                   GlobalEnvironment& environment,
                                   std::shared_ptr<Storage> pStorage,
                                   std::promise<void>& promise) {

    auto[_, callback] = createAsyncQuery<std::unique_ptr<Folder>>([=, &environment, &promise](auto&& res) {
        ASSERT(res, environment.logger())
        onFilesystemReceived(driveKey, environment, pStorage, promise, std::move(*res));
    }, [] {}, environment, false, true);
    pStorage->filesystem(driveKey, callback);
}

void onEvaluatedStorageHash(const DriveKey& driveKey,
                            GlobalEnvironment& environment,
                            std::shared_ptr<Storage> pStorage,
                            std::promise<void>& barrier,
                            const StorageState& digest) {
    auto[_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT(res, environment.logger())
        onAppliedStorageModifications(driveKey, environment, pStorage, barrier);
    }, [] {}, environment, false, true);
    pStorage->applyStorageModifications(driveKey, true, callback);
}

void onAppliedSandboxModifications(const DriveKey& driveKey,
                                   GlobalEnvironment& environment,
                                   std::shared_ptr<Storage> pStorage,
                                   std::promise<void>& barrier,
                                   const SandboxModificationDigest& digest) {
    auto[_, callback] = createAsyncQuery<StorageState>([=, &environment, &barrier](auto&& res) {
        ASSERT(res, environment.logger())
        onEvaluatedStorageHash(driveKey, environment, pStorage, barrier, *res);
    }, [] {}, environment, false, true);

    pStorage->evaluateStorageHash(driveKey, callback);
}

void onClosedFile(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  std::shared_ptr<Storage> pStorage,
                  std::promise<void>& barrier) {
    auto[_, callback] = createAsyncQuery<SandboxModificationDigest>([=, &environment, &barrier](auto&& res) {
        ASSERT(res, environment.logger())
        onAppliedSandboxModifications(driveKey, environment, pStorage, barrier, *res);
    }, [] {}, environment, false, true);

    pStorage->applySandboxStorageModifications(driveKey, true, callback);
}

void onWrittenFile(const DriveKey& driveKey,
                   GlobalEnvironment& environment,
                   std::shared_ptr<Storage> pStorage,
                   std::promise<void>& barrier,
                   uint64_t fileId) {
    auto[_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT(res, environment.logger())
        onClosedFile(driveKey, environment, pStorage, barrier);
    }, [] {}, environment, false, true);

    pStorage->closeFile(driveKey, fileId, callback);
}

void onOpenedFile(const DriveKey& driveKey,
                  GlobalEnvironment& environment,
                  std::shared_ptr<Storage> pStorage,
                  std::promise<void>& barrier,
                  uint64_t fileId) {
    auto[_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT(res, environment.logger())
        onWrittenFile(driveKey, environment, pStorage, barrier, fileId);
    }, [] {}, environment, false, true);

    std::string s("data");
    pStorage->writeFile(driveKey, fileId, {s.begin(), s.end()}, callback);
}

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     std::shared_ptr<Storage> pStorage,
                                     std::promise<void>& barrier) {
    auto[_, callback] = createAsyncQuery<uint64_t>([=, &environment, &barrier](auto&& res) {
        ASSERT(res, environment.logger())
        onOpenedFile(driveKey, environment, pStorage, barrier, *res);
    }, [] {}, environment, false, true);

    pStorage->openFile(driveKey, "test.txt", OpenFileMode::WRITE, callback);
}

void onModificationsInitiated(const DriveKey& driveKey,
                              GlobalEnvironment& environment,
                              std::shared_ptr<Storage> pStorage,
                              std::promise<void>& barrier) {
    auto[_, callback] = createAsyncQuery<void>([=, &environment, &barrier](auto&& res) {
        ASSERT(res, environment.logger())
        onSandboxModificationsInitiated(driveKey, environment, pStorage, barrier);
    }, [] {}, environment, false, true);

    pStorage->initiateSandboxModifications(driveKey, callback);
}

TEST(Storage, Example) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    std::promise<void> p;
    auto barrier = p.get_future();

    DriveKey driveKey{{1}};

    threadManager.execute([&] {
        // TODO Fill in the address
        std::string address = "127.0.0.1:5551";

        auto pStorage = std::make_shared<RPCStorage>(environment, address);

        auto[_, callback] = createAsyncQuery<void>([=, &environment, &p](auto&& res) {
            ASSERT(res, environment.logger())
            onModificationsInitiated(driveKey, environment, pStorage, p);
        }, [] {}, environment, false, true);
        pStorage->initiateModifications(driveKey, callback);
    });

    barrier.get();

    threadManager.stop();
}
}