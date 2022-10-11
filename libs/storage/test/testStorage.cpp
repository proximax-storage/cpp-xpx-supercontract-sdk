/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <gtest/gtest.h>
#include <storage/RPCStorage.h>
#include "TestUtils.h"

namespace sirius::contract::storage::test {

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

        auto storage = pStorage;

        auto[_, callback] = createAsyncQuery<void>([=, &environment, &p](auto&& res) {
            ASSERT(res, environment.logger())
            onModificationsInitiated(driveKey, environment, storage, p);
        }, [] {}, environment, false, true);
        pStorage->initiateModifications(driveKey, callback);
    });

    barrier.get();

    threadManager.stop();
}
}