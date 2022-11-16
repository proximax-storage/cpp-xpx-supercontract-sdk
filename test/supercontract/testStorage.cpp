#include "ContractEnvironmentMock.h"
#include "ExecutorEnvironmentMock.h"
#include "TestUtils.h"
#include "storage/RPCStorage.h"
#include "storage/StorageErrorCode.h"
#include "supercontract/CallExecutionEnvironment.h"
#include "virtualMachine/RPCVirtualMachineBuilder.h"
#include "virtualMachine/VirtualMachine.h"
#include <gtest/gtest.h>

namespace sirius::contract::test {
// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
std::string exec(const char* cmd) {
    std::array<char, 128> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    std::cout << "exec result " << result;
    return result;
}

TEST(Supercontract, Storage) {
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    executorConfig.setRpcStorageAddress("127.0.0.1:5551");
    ThreadManager threadManager;
    std::shared_ptr<vm::VirtualMachine> pVirtualMachine;
    ExecutorEnvironmentMock environment(std::move(keyPair), std::move(pVirtualMachine), executorConfig,
                                        threadManager);

    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    ContractEnvironmentMock contractEnvironmentMock(environment, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);
    contractEnvironmentMock.m_driveKey = DriveKey({1});

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<CallExecutionEnvironment> rpcHandler;
    std::shared_ptr<storage::Storage> pStorage;

    std::promise<void> pInit;
    auto barrierInit = pInit.get_future();

    threadManager.execute([&] {
        pStorage = std::make_shared<storage::RPCStorage>(environment, executorConfig.rpcStorageAddress());
        environment.m_storage = pStorage;
        auto[_, storageCallback] = createAsyncQuery<void>(
                [=, &environment, &contractEnvironmentMock, &pInit](auto&& res) {
                    auto[_, sandboxCallback] = createAsyncQuery<void>([&pInit](auto&& res) { pInit.set_value(); },
                                                                      [] {}, environment, false, true);
                    pStorage->initiateSandboxModifications(contractEnvironmentMock.driveKey(), sandboxCallback);
                },
                [] {}, environment, false, true);
        pStorage->initiateModifications(contractEnvironmentMock.driveKey(), storageCallback);
    });

    barrierInit.get();

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // exec("cp ../../libs/virtualMachine/test/supercontracts/simple.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/storage.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        vm::RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);
        environment.m_virtualMachineMock = pVirtualMachine;

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        vm::CallRequest callRequest = vm::CallRequest(CallRequestParameters{
                                                              ContractKey(),
                                                              CallId(),
                                                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                                                              "run",
                                                              params,
                                                              25000000000,
                                                              26 * 1024,
                                                              CallReferenceInfo{
                                                                      {},
                                                                      0,
                                                                      BlockHash(),
                                                                      0,
                                                                      0,
                                                                      {}}},
                                                      vm::CallRequest::CallLevel::MANUAL);

        rpcHandler = std::make_shared<CallExecutionEnvironment>(callRequest, environment, contractEnvironmentMock);

        auto[_, callback] = createAsyncQuery<vm::CallExecutionResult>([&](auto&& res) {
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1);
            ASSERT_EQ(res->m_scConsumed, 4402853086);
            ASSERT_EQ(res->m_smConsumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, rpcHandler, rpcHandler, rpcHandler, callback);
    });

    barrier.get();

    std::promise<void> pApply;
    auto barrierApply = pApply.get_future();

    threadManager.execute([&] {
        auto[q1, applySandboxCallback] = createAsyncQuery<storage::SandboxModificationDigest>(
                [=, &environment, &contractEnvironmentMock, &pApply](auto&& res) {
                    auto[q2, evaluateStateCallback] = createAsyncQuery<storage::StorageState>(
                            [=, &environment, &contractEnvironmentMock, &pApply](auto&& res) {
                                auto[q3, applyStorageCallback] = createAsyncQuery<void>(
                                        [&pApply](auto&& res) { pApply.set_value(); }, [] {}, environment, false, true);
                                pStorage->applyStorageModifications(contractEnvironmentMock.driveKey(), true,
                                                                    applyStorageCallback);
                            },
                            [] {}, environment, false, true);
                    pStorage->evaluateStorageHash(contractEnvironmentMock.driveKey(), evaluateStateCallback);
                },
                [] {}, environment, false, true);

        pStorage->applySandboxStorageModifications(contractEnvironmentMock.driveKey(), true, applySandboxCallback);
    });

    barrierApply.get();

    threadManager.execute([&] {
        pStorage.reset();
        pVirtualMachine.reset();
    });

    threadManager.stop();
    // exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(Supercontract, Iterator) {
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    executorConfig.setRpcStorageAddress("127.0.0.1:5551");
    ThreadManager threadManager;
    std::shared_ptr<vm::VirtualMachine> pVirtualMachine;
    ExecutorEnvironmentMock environment(std::move(keyPair), std::move(pVirtualMachine), executorConfig,
                                        threadManager);

    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    ContractEnvironmentMock contractEnvironmentMock(environment, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);
    contractEnvironmentMock.m_driveKey = DriveKey({2});

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<CallExecutionEnvironment> rpcHandler;
    std::shared_ptr<storage::Storage> pStorage;

    std::promise<void> pInit;
    auto barrierInit = pInit.get_future();

    threadManager.execute([&] {
        pStorage = std::make_shared<storage::RPCStorage>(environment, executorConfig.rpcStorageAddress());
        environment.m_storage = pStorage;
        auto[_, storageCallback] = createAsyncQuery<void>(
                [=, &environment, &contractEnvironmentMock, &pInit](auto&& res) {
                    auto[_, sandboxCallback] = createAsyncQuery<void>([&pInit](auto&& res) { pInit.set_value(); },
                                                                      [] {}, environment, false, true);
                    pStorage->initiateSandboxModifications(contractEnvironmentMock.driveKey(), sandboxCallback);
                },
                [] {}, environment, false, true);
        pStorage->initiateModifications(contractEnvironmentMock.driveKey(), storageCallback);
    });

    barrierInit.get();

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // exec("cp ../../libs/virtualMachine/test/supercontracts/simple.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/iterator.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        vm::RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);
        environment.m_virtualMachineMock = pVirtualMachine;

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        vm::CallRequest callRequest = vm::CallRequest(CallRequestParameters{
                                                              ContractKey(),
                                                              CallId(),
                                                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                                                              "run",
                                                              params,
                                                              25000000000,
                                                              26 * 1024,
                                                              CallReferenceInfo{
                                                                      {},
                                                                      0,
                                                                      BlockHash(),
                                                                      0,
                                                                      0,
                                                                      {}}},
                                                      vm::CallRequest::CallLevel::MANUAL);

        rpcHandler = std::make_shared<CallExecutionEnvironment>(callRequest, environment, contractEnvironmentMock);

        auto[_, callback] = createAsyncQuery<vm::CallExecutionResult>([&](auto&& res) {
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1);
            ASSERT_EQ(res->m_scConsumed, 11693600352);
            ASSERT_EQ(res->m_smConsumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, rpcHandler, rpcHandler, rpcHandler, callback);
    });

    barrier.get();

    std::promise<void> pApply;
    auto barrierApply = pApply.get_future();

    threadManager.execute([&] {
        auto[q1, applySandboxCallback] = createAsyncQuery<storage::SandboxModificationDigest>(
                [=, &environment, &contractEnvironmentMock, &pApply](auto&& res) {
                    auto[q2, evaluateStateCallback] = createAsyncQuery<storage::StorageState>(
                            [=, &environment, &contractEnvironmentMock, &pApply](auto&& res) {
                                auto[q3, applyStorageCallback] = createAsyncQuery<void>(
                                        [&pApply](auto&& res) { pApply.set_value(); }, [] {}, environment, false, true);
                                pStorage->applyStorageModifications(contractEnvironmentMock.driveKey(), true,
                                                                    applyStorageCallback);
                            },
                            [] {}, environment, false, true);
                    pStorage->evaluateStorageHash(contractEnvironmentMock.driveKey(), evaluateStateCallback);
                },
                [] {}, environment, false, true);

        pStorage->applySandboxStorageModifications(contractEnvironmentMock.driveKey(), true, applySandboxCallback);
    });

    barrierApply.get();

    threadManager.execute([&] {
        pStorage.reset();
        pVirtualMachine.reset();
    });

    threadManager.stop();
    // exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(Supercontract, FaultyStorage) {
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    executorConfig.setRpcStorageAddress("127.0.0.1:5551");
    ThreadManager threadManager;
    std::shared_ptr<vm::VirtualMachine> pVirtualMachine;
    ExecutorEnvironmentMock environment(std::move(keyPair), std::move(pVirtualMachine), executorConfig,
                                        threadManager);

    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    ContractEnvironmentMock contractEnvironmentMock(environment, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);
    contractEnvironmentMock.m_driveKey = DriveKey({3});

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<CallExecutionEnvironment> rpcHandler;
    std::shared_ptr<storage::Storage> pStorage;

    std::promise<void> pInit;
    auto barrierInit = pInit.get_future();

    threadManager.execute([&] {
        pStorage = std::make_shared<storage::RPCStorage>(environment, executorConfig.rpcStorageAddress());
        environment.m_storage = pStorage;
        auto[_, storageCallback] = createAsyncQuery<void>(
                [=, &environment, &contractEnvironmentMock, &pInit](auto&& res) {
                    auto[_, sandboxCallback] = createAsyncQuery<void>([&pInit](auto&& res) { pInit.set_value(); },
                                                                      [] {}, environment, false, true);
                    pStorage->initiateSandboxModifications(contractEnvironmentMock.driveKey(), sandboxCallback);
                },
                [] {}, environment, false, true);
        pStorage->initiateModifications(contractEnvironmentMock.driveKey(), storageCallback);
    });

    barrierInit.get();

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // exec("cp ../../libs/virtualMachine/test/supercontracts/simple.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/storage_faulty.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        vm::RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);
        environment.m_virtualMachineMock = pVirtualMachine;

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        vm::CallRequest callRequest = vm::CallRequest(CallRequestParameters{
                                                              ContractKey(),
                                                              CallId(),
                                                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                                                              "run",
                                                              params,
                                                              25000000000,
                                                              26 * 1024,
                                                              CallReferenceInfo{
                                                                      {},
                                                                      0,
                                                                      BlockHash(),
                                                                      0,
                                                                      0,
                                                                      {}}},
                                                      vm::CallRequest::CallLevel::MANUAL);

        rpcHandler = std::make_shared<CallExecutionEnvironment>(callRequest, environment, contractEnvironmentMock);

        auto[_, callback] = createAsyncQuery<vm::CallExecutionResult>([&](auto&& res) {
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1);
            ASSERT_EQ(res->m_scConsumed, 1437180);
            ASSERT_EQ(res->m_smConsumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, rpcHandler, rpcHandler, rpcHandler, callback);
    });

    barrier.get();

    std::promise<void> pApply;
    auto barrierApply = pApply.get_future();

    threadManager.execute([&] {
        auto[q1, applySandboxCallback] = createAsyncQuery<storage::SandboxModificationDigest>(
                [=, &environment, &contractEnvironmentMock, &pApply](auto&& res) {
                    auto[q2, evaluateStateCallback] = createAsyncQuery<storage::StorageState>(
                            [=, &environment, &contractEnvironmentMock, &pApply](auto&& res) {
                                auto[q3, applyStorageCallback] = createAsyncQuery<void>(
                                        [&pApply](auto&& res) { pApply.set_value(); }, [] {}, environment, false, true);
                                pStorage->applyStorageModifications(contractEnvironmentMock.driveKey(), true,
                                                                    applyStorageCallback);
                            },
                            [] {}, environment, false, true);
                    pStorage->evaluateStorageHash(contractEnvironmentMock.driveKey(), evaluateStateCallback);
                },
                [] {}, environment, false, true);

        pStorage->applySandboxStorageModifications(contractEnvironmentMock.driveKey(), true, applySandboxCallback);
    });

    barrierApply.get();

    threadManager.execute([&] {
        pStorage.reset();
        pVirtualMachine.reset();
    });

    threadManager.stop();
    // exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

} // namespace sirius::contract::test