#include "ContractEnvironmentMock.h"
#include "ExecutorEnvironmentMock.h"
#include "StorageMock.h"
#include <storage/RPCStorageBuilder.h>
#include "storage/StorageErrorCode.h"
#include "virtualMachine/RPCVirtualMachineBuilder.h"
#include "virtualMachine/VirtualMachine.h"
#include <gtest/gtest.h>
#include <utils/Random.h>
#include <InternetQueryHandler.h>
#include <StorageQueryHandler.h>
#include <ManualCallBlockchainQueryHandler.h>
#include <boost/process.hpp>

namespace sirius::contract::test {

namespace {

struct StorageModificationHolder {
    std::unique_ptr<storage::StorageModification> m_storageModification;
    std::shared_ptr<storage::SandboxModification> m_sandboxModification;
};

void exec(const char* cmd) {
    boost::process::child c(cmd);
    c.join();
    ASSERT_EQ(c.exit_code(), 0);
}

std::optional<std::string> storageAddress() {
#ifdef SIRIUS_CONTRACT_STORAGE_ADDRESS_TEST
    return  SIRIUS_CONTRACT_STORAGE_ADDRESS_TEST;
#else
    return {};
#endif
};

std::optional<std::string> vmAddress() {
#ifdef SIRIUS_CONTRACT_VM_ADDRESS_TEST
    return SIRIUS_CONTRACT_VM_ADDRESS_TEST;
#else
    return {};
#endif
};

}

TEST(Supercontract, Storage) {

    auto rpcStorageAddress = storageAddress();

    if (!rpcStorageAddress) {
        GTEST_SKIP();
    }

    auto rpcVirtualMachineAddress = vmAddress();

    if (!rpcVirtualMachineAddress) {
        GTEST_SKIP();
    }

    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    auto threadManager = std::make_shared<ThreadManager>();
    std::shared_ptr<vm::VirtualMachine> pVirtualMachine;
    ExecutorEnvironmentMock environment(std::move(keyPair), std::move(pVirtualMachine), executorConfig,
                                        threadManager);

    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    ContractEnvironmentMock contractEnvironmentMock(environment, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);
    contractEnvironmentMock.m_driveKey = utils::generateRandomByteValue<DriveKey>();

    auto storageObserver = std::make_shared<StorageMock>();

    std::shared_ptr<vm::VirtualMachineInternetQueryHandler> internetHandler;
    std::shared_ptr<vm::VirtualMachineStorageQueryHandler> storageHandler;
    std::shared_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainHandler;
    std::shared_ptr<storage::Storage> pStorage;

    StorageModificationHolder modificationHolder;

    std::promise<void> pInit;
    auto barrierInit = pInit.get_future();

    environment.threadManager().execute([&] {
        pStorage = storage::RPCStorageBuilder(*rpcStorageAddress).build(environment);
        environment.m_storage = pStorage;
        auto[_, storageCallback] = createAsyncQuery<std::unique_ptr<storage::StorageModification>>(
                [=, &environment, &modificationHolder, &contractEnvironmentMock, &pInit](auto&& res) {
                    ASSERT_TRUE(res);
                    modificationHolder.m_storageModification = std::move(*res);
                    auto[_, sandboxCallback] = createAsyncQuery<std::unique_ptr<storage::SandboxModification>>(
                            [&pInit, &modificationHolder](auto&& res) mutable {
                                ASSERT_TRUE(res);
                                modificationHolder.m_sandboxModification = std::move(*res);
                                pInit.set_value();
                            },
                            [] {}, environment, false, true);
                    modificationHolder.m_storageModification->initiateSandboxModification(sandboxCallback);
                },
                [] {}, environment, false, true);
        pStorage->initiateModifications(contractEnvironmentMock.driveKey(),
                                        utils::generateRandomByteValue<ModificationId>(), storageCallback);
    });

    ASSERT_EQ(std::future_status::ready, barrierInit.wait_for(std::chrono::seconds(10)));

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    environment.threadManager().execute([&] {
        std::filesystem::copy("supercontracts/storage.rs",
                              "rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");
        vm::RPCVirtualMachineBuilder builder(*rpcVirtualMachineAddress);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);
        environment.m_virtualMachineMock = pVirtualMachine;


        std::vector<uint8_t> params;
        vm::CallRequest callRequest = vm::CallRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                26 * 1024,
                vm::CallRequest::CallLevel::MANUAL, 0);

        internetHandler = std::make_shared<InternetQueryHandler>(callRequest.m_callId,
                                                                 executorConfig.maxInternetConnections(),
                                                                 environment,
                                                                 contractEnvironmentMock);
        storageHandler = std::make_shared<StorageQueryHandler>(callRequest.m_callId,
                                                               environment,
                                                               contractEnvironmentMock,
                                                               modificationHolder.m_sandboxModification);
        blockchainHandler = std::make_shared<ManualCallBlockchainQueryHandler>(environment,
                                                                               contractEnvironmentMock,
                                                                               CallerKey(),
                                                                               0,
                                                                               25000000000,
                                                                               26 * 1024,
                                                                               callRequest.m_callId.array(),
                                                                               std::vector<ServicePayment>());

        auto[_, callback] = createAsyncQuery<vm::CallExecutionResult>([&](auto&& res) {
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1);
            ASSERT_EQ(res->m_execution_gas_consumed, 4402891458);
            ASSERT_EQ(res->m_download_gas_consumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler, blockchainHandler, storageHandler, callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(60)));

    std::promise<void> pApply;
    auto barrierApply = pApply.get_future();

    environment.threadManager().execute([&] {
        auto[q1, applySandboxCallback] = createAsyncQuery<storage::SandboxModificationDigest>(
                [=, &environment, &modificationHolder, &contractEnvironmentMock, &pApply](auto&& res) {
                    auto[q2, evaluateStateCallback] = createAsyncQuery<storage::StorageState>(
                            [=, &environment, &modificationHolder, &contractEnvironmentMock, &pApply](auto&& res) {
                                auto[q3, applyStorageCallback] = createAsyncQuery<void>(
                                        [&pApply](auto&& res) { pApply.set_value(); }, [] {}, environment, false, true);
                                modificationHolder.m_storageModification->applyStorageModification(true,
                                                                                                   applyStorageCallback);
                            },
                            [] {}, environment, false, true);
                    modificationHolder.m_storageModification->evaluateStorageHash(evaluateStateCallback);
                },
                [] {}, environment, false, true);

        modificationHolder.m_sandboxModification->applySandboxModification(true, applySandboxCallback);
    });

    ASSERT_EQ(std::future_status::ready, barrierApply.wait_for(std::chrono::seconds(10)));

    environment.threadManager().execute([&] {
        pStorage.reset();
        modificationHolder.m_storageModification.reset();
        modificationHolder.m_sandboxModification.reset();
        pVirtualMachine.reset();
    });

    environment.threadManager().stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(Supercontract, Iterator) {

    auto rpcStorageAddress = storageAddress();

    if (!rpcStorageAddress) {
        GTEST_SKIP();
    }

    auto rpcVirtualMachineAddress = vmAddress();

    if (!rpcVirtualMachineAddress) {
        GTEST_SKIP();
    }

    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    auto threadManager = std::make_shared<ThreadManager>();
    std::shared_ptr<vm::VirtualMachine> pVirtualMachine;
    ExecutorEnvironmentMock environment(std::move(keyPair), std::move(pVirtualMachine), executorConfig,
                                        threadManager);

    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    ContractEnvironmentMock contractEnvironmentMock(environment, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);
    contractEnvironmentMock.m_driveKey = utils::generateRandomByteValue<DriveKey>();

    auto storageObserver = std::make_shared<StorageMock>();

    std::shared_ptr<vm::VirtualMachineInternetQueryHandler> internetHandler;
    std::shared_ptr<vm::VirtualMachineStorageQueryHandler> storageHandler;
    std::shared_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainHandler;
    std::shared_ptr<storage::Storage> pStorage;

    StorageModificationHolder modificationHolder;

    std::promise<void> pInit;
    auto barrierInit = pInit.get_future();

    environment.threadManager().execute([&] {
        pStorage = storage::RPCStorageBuilder(*rpcStorageAddress).build(environment);
        environment.m_storage = pStorage;
        auto[_, storageCallback] = createAsyncQuery<std::unique_ptr<storage::StorageModification>>(
                [=, &environment, &modificationHolder, &contractEnvironmentMock, &pInit](auto&& res) {
                    ASSERT_TRUE(res);
                    modificationHolder.m_storageModification = std::move(*res);
                    auto[_, sandboxCallback] = createAsyncQuery<std::unique_ptr<storage::SandboxModification>>(
                            [&pInit, &modificationHolder](auto&& res) {
                                ASSERT_TRUE(res);
                                modificationHolder.m_sandboxModification = std::move(*res);
                                pInit.set_value();
                            },
                            [] {}, environment, false, true);
                    modificationHolder.m_storageModification->initiateSandboxModification(sandboxCallback);
                },
                [] {}, environment, false, true);
        pStorage->initiateModifications(contractEnvironmentMock.driveKey(),
                                        utils::generateRandomByteValue<ModificationId>(), storageCallback);
    });

    ASSERT_EQ(std::future_status::ready, barrierInit.wait_for(std::chrono::seconds(10)));

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    environment.threadManager().execute([&] {
        std::filesystem::copy("supercontracts/iterator.rs",
                              "rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");
        vm::RPCVirtualMachineBuilder builder(*rpcVirtualMachineAddress);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);
        environment.m_virtualMachineMock = pVirtualMachine;

        std::vector<uint8_t> params;
        vm::CallRequest callRequest = vm::CallRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                26 * 1024,
                vm::CallRequest::CallLevel::MANUAL, 0);

        internetHandler = std::make_shared<InternetQueryHandler>(callRequest.m_callId,
                                                                 executorConfig.maxInternetConnections(),
                                                                 environment,
                                                                 contractEnvironmentMock);
        storageHandler = std::make_shared<StorageQueryHandler>(callRequest.m_callId,
                                                               environment,
                                                               contractEnvironmentMock,
                                                               modificationHolder.m_sandboxModification);
        blockchainHandler = std::make_shared<ManualCallBlockchainQueryHandler>(environment,
                                                                               contractEnvironmentMock,
                                                                               CallerKey(),
                                                                               0,
                                                                               25000000000,
                                                                               26 * 1024,
                                                                               callRequest.m_callId.array(),
                                                                               std::vector<ServicePayment>());

        auto[_, callback] = createAsyncQuery<vm::CallExecutionResult>([&](auto&& res) {
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1);
            ASSERT_EQ(res->m_execution_gas_consumed, 218275185);
            ASSERT_EQ(res->m_download_gas_consumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler, blockchainHandler, storageHandler, callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(60)));

    std::promise<void> pApply;
    auto barrierApply = pApply.get_future();

    environment.threadManager().execute([&] {
        auto[q1, applySandboxCallback] = createAsyncQuery<storage::SandboxModificationDigest>(
                [=, &environment, &modificationHolder, &contractEnvironmentMock, &pApply](auto&& res) {
                    auto[q2, evaluateStateCallback] = createAsyncQuery<storage::StorageState>(
                            [=, &environment, &modificationHolder, &contractEnvironmentMock, &pApply](auto&& res) {
                                auto[q3, applyStorageCallback] = createAsyncQuery<void>(
                                        [&pApply](auto&& res) { pApply.set_value(); }, [] {}, environment, false, true);
                                modificationHolder.m_storageModification->applyStorageModification(true,
                                                                                                   applyStorageCallback);
                            },
                            [] {}, environment, false, true);
                    modificationHolder.m_storageModification->evaluateStorageHash(evaluateStateCallback);
                },
                [] {}, environment, false, true);

        modificationHolder.m_sandboxModification->applySandboxModification(true, applySandboxCallback);
    });

    ASSERT_EQ(std::future_status::ready, barrierApply.wait_for(std::chrono::seconds(10)));

    environment.threadManager().execute([&] {
        pStorage.reset();
        modificationHolder.m_storageModification.reset();
        modificationHolder.m_sandboxModification.reset();
        pVirtualMachine.reset();
    });

    environment.threadManager().stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(Supercontract, FaultyStorage) {

    auto rpcStorageAddress = storageAddress();

    if (!rpcStorageAddress) {
        GTEST_SKIP();
    }

    auto rpcVirtualMachineAddress = vmAddress();

    if (!rpcVirtualMachineAddress) {
        GTEST_SKIP();
    }

    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    auto threadManager = std::make_shared<ThreadManager>();
    std::shared_ptr<vm::VirtualMachine> pVirtualMachine;
    ExecutorEnvironmentMock environment(std::move(keyPair), std::move(pVirtualMachine), executorConfig,
                                        threadManager);

    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    ContractEnvironmentMock contractEnvironmentMock(environment, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);
    contractEnvironmentMock.m_driveKey = utils::generateRandomByteValue<DriveKey>();

    auto storageObserver = std::make_shared<StorageMock>();

    std::shared_ptr<vm::VirtualMachineInternetQueryHandler> internetHandler;
    std::shared_ptr<vm::VirtualMachineStorageQueryHandler> storageHandler;
    std::shared_ptr<vm::VirtualMachineBlockchainQueryHandler> blockchainHandler;
    std::shared_ptr<storage::Storage> pStorage;

    StorageModificationHolder modificationHolder;

    std::promise<void> pInit;
    auto barrierInit = pInit.get_future();

    environment.threadManager().execute([&] {
        pStorage = storage::RPCStorageBuilder(*rpcStorageAddress).build(environment);
        environment.m_storage = pStorage;
        auto[_, storageCallback] = createAsyncQuery<std::unique_ptr<storage::StorageModification>>(
                [=, &environment, &modificationHolder, &contractEnvironmentMock, &pInit](auto&& res) {
                    ASSERT_TRUE(res);
                    modificationHolder.m_storageModification = std::move(*res);
                    auto[_, sandboxCallback] = createAsyncQuery<std::unique_ptr<storage::SandboxModification>>(
                            [&pInit, &modificationHolder](auto&& res) {
                                ASSERT_TRUE(res);
                                modificationHolder.m_sandboxModification = std::move(*res);
                                pInit.set_value();
                            },
                            [] {}, environment, false, true);
                    modificationHolder.m_storageModification->initiateSandboxModification(sandboxCallback);
                },
                [] {}, environment, false, true);
        pStorage->initiateModifications(contractEnvironmentMock.driveKey(),
                                        utils::generateRandomByteValue<ModificationId>(), storageCallback);
    });

    ASSERT_EQ(std::future_status::ready, barrierInit.wait_for(std::chrono::seconds(10)));

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    environment.threadManager().execute([&] {
        std::filesystem::copy("supercontracts/storage_faulty.rs",
                              "rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");
        vm::RPCVirtualMachineBuilder builder(*rpcVirtualMachineAddress);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);
        environment.m_virtualMachineMock = pVirtualMachine;

        std::vector<uint8_t> params;
        vm::CallRequest callRequest = vm::CallRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                26 * 1024,
                vm::CallRequest::CallLevel::MANUAL, 0);

        internetHandler = std::make_shared<InternetQueryHandler>(callRequest.m_callId,
                                                                 executorConfig.maxInternetConnections(),
                                                                 environment,
                                                                 contractEnvironmentMock);
        storageHandler = std::make_shared<StorageQueryHandler>(callRequest.m_callId,
                                                               environment,
                                                               contractEnvironmentMock,
                                                               modificationHolder.m_sandboxModification);
        blockchainHandler = std::make_shared<ManualCallBlockchainQueryHandler>(environment,
                                                                               contractEnvironmentMock,
                                                                               CallerKey(),
                                                                               0,
                                                                               25000000000,
                                                                               26 * 1024,
                                                                               callRequest.m_callId.array(),
                                                                               std::vector<ServicePayment>());

        auto[_, callback] = createAsyncQuery<vm::CallExecutionResult>([&](auto&& res) {
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1);
            ASSERT_EQ(res->m_execution_gas_consumed, 1437180);
            ASSERT_EQ(res->m_download_gas_consumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler, blockchainHandler, storageHandler, callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(60)));

    std::promise<void> pApply;
    auto barrierApply = pApply.get_future();

    environment.threadManager().execute([&] {
        auto[q1, applySandboxCallback] = createAsyncQuery<storage::SandboxModificationDigest>(
                [=, &environment, &modificationHolder, &contractEnvironmentMock, &pApply](auto&& res) {
                    auto[q2, evaluateStateCallback] = createAsyncQuery<storage::StorageState>(
                            [=, &environment, &modificationHolder, &contractEnvironmentMock, &pApply](auto&& res) {
                                auto[q3, applyStorageCallback] = createAsyncQuery<void>(
                                        [&pApply](auto&& res) { pApply.set_value(); }, [] {}, environment, false, true);
                                modificationHolder.m_storageModification->applyStorageModification(true,
                                                                                                   applyStorageCallback);
                            },
                            [] {}, environment, false, true);
                    modificationHolder.m_storageModification->evaluateStorageHash(evaluateStateCallback);
                },
                [] {}, environment, false, true);

        modificationHolder.m_sandboxModification->applySandboxModification(true, applySandboxCallback);
    });

    ASSERT_EQ(std::future_status::ready, barrierApply.wait_for(std::chrono::seconds(10)));

    environment.threadManager().execute([&] {
        pStorage.reset();
        modificationHolder.m_storageModification.reset();
        modificationHolder.m_sandboxModification.reset();
        pVirtualMachine.reset();
    });

    environment.threadManager().stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

} // namespace sirius::contract::test