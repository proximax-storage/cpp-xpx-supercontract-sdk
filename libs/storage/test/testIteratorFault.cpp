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

void onIteratorCreated(const DriveKey& driveKey,
                       GlobalEnvironment& environment,
                       ContextHolder& contextHolder,
                       std::promise<void>& barrier, uint64_t id) {
    auto [_, callback] = createAsyncQuery<SandboxModificationDigest>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_TRUE(res);
        onAppliedSandboxModifications(driveKey, environment, contextHolder, barrier, *res); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->applySandboxModification(true, callback);
}

void onSandboxModificationsInitiated(const DriveKey& driveKey,
                                     GlobalEnvironment& environment,
                                     ContextHolder& contextHolder,
                                     std::promise<void>& barrier) {
    auto [_, callback] = createAsyncQuery<uint64_t>([=, &environment, &contextHolder, &barrier](auto&& res) {
        ASSERT_EQ(res.error(), StorageError::create_iterator_error);
        onIteratorCreated(driveKey, environment, contextHolder, barrier, *res); }, [] {}, environment, false, true);

    contextHolder.m_sandboxModification->directoryIteratorCreate("tests", true, callback);
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

TEST(Storage, IteratorFault) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    ContextHolder contextHolder;

    DriveKey driveKey{{9}};

    std::promise<void> pIterate;
    auto barrierIterate = pIterate.get_future();

    threadManager.execute([&] {
        std::string address = "127.0.0.1:5551";

        contextHolder.m_storage = std::make_unique<RPCStorage>(environment, address);

        auto [_, callback] = createAsyncQuery<std::unique_ptr<StorageModification>>([=, &environment, &contextHolder, &pIterate](auto&& res) {
            ASSERT_TRUE(res);
            contextHolder.m_storageModification = std::move(*res);
            onModificationsInitiated(driveKey, environment, contextHolder, pIterate); }, [] {}, environment, false, true);
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