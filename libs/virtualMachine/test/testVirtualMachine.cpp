/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "MockInternetHandler.h"
#include "TestUtils.h"
#include <gtest/gtest.h>
#include <virtualMachine/RPCVirtualMachineBuilder.h>

namespace sirius::contract::vm::test {
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

TEST(VirtualMachine, SimpleContract) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // exec("cp ../../libs/virtualMachine/test/supercontracts/simple.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/simple.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        vm::CallRequest callRequest = CallRequest(CallRequestParameters{
                ContractKey(),
                CallId(),
                "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                52000000,
                20 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        BlockHash(),
                        0,
                        0,
                        {}
                }
        }, CallRequest::CallLevel::AUTOMATIC);

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1 + 1);
            ASSERT_EQ(res->m_scConsumed, 604);
            ASSERT_EQ(res->m_smConsumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, std::weak_ptr<VirtualMachineInternetQueryHandler>(),
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    barrier.get();

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    // exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, InternetRead) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/internet_read.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest(CallRequestParameters{
                ContractKey(),
                CallId(),
                "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        BlockHash(),
                        0,
                        0,
                        {}
                }
        }, CallRequest::CallLevel::MANUAL);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_TRUE(res);
            // std::cout << res->m_success << std::endl;
            // std::cout << res->m_return << std::endl;
            // std::cout << res->m_scConsumed << std::endl;
            // std::cout << res->m_smConsumed << std::endl;
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1);
            ASSERT_EQ(res->m_scConsumed, 20578109322);
            ASSERT_EQ(res->m_smConsumed, 10240);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    barrier.get();

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    // exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, InternetReadNotEnoughSC) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/internet_read.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest(CallRequestParameters{
                ContractKey(),
                CallId(),
                "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                100000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        BlockHash(),
                        0,
                        0,
                        {}
                }
        }, CallRequest::CallLevel::MANUAL);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_scConsumed, 100000);
            ASSERT_EQ(res->m_smConsumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    barrier.get();

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, InternetReadNotEnoughSM) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/internet_read.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest(CallRequestParameters{
                ContractKey(),
                CallId(),
                "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                25 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        BlockHash(),
                        0,
                        0,
                        {}
                }
        }, CallRequest::CallLevel::MANUAL);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_scConsumed, 1484710915);
            ASSERT_EQ(res->m_smConsumed, 10 * 1024);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    barrier.get();

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, WrongContractPath) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/internet_read.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest(CallRequestParameters{
                ContractKey(),
                CallId(),
                "sdk_bg.wasm",
                "run",
                params,
                25000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        BlockHash(),
                        0,
                        0,
                        {}
                }
        }, CallRequest::CallLevel::MANUAL);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_FALSE(res);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    barrier.get();

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, WrongIP) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/internet_read.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50052";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest(CallRequestParameters{
                ContractKey(),
                CallId(),
                "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        BlockHash(),
                        0,
                        0,
                        {}
                }
        }, CallRequest::CallLevel::MANUAL);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_FALSE(res);
            ASSERT_TRUE(res.error() == std::make_error_code(std::errc::connection_aborted));
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    barrier.get();

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, WrongExecFunction) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/internet_read.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest(CallRequestParameters{
                ContractKey(),
                CallId(),
                "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "runs",
                params,
                25000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        BlockHash(),
                        0,
                        0,
                        {}
                }
        }, CallRequest::CallLevel::MANUAL);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_scConsumed, 0);
            ASSERT_EQ(res->m_smConsumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    barrier.get();

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, UnauthorizedImportFunction) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/internet_read.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest(CallRequestParameters{
                ContractKey(),
                CallId(),
                "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        BlockHash(),
                        0,
                        0,
                        {}
                }
        }, CallRequest::CallLevel::AUTOMATIC);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_scConsumed, 1096579);
            ASSERT_EQ(res->m_smConsumed, 0); // Internet read function shouldn't be called, so no SM is consumed
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    barrier.get();

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, AbortVMDuringExecution) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // exec("cp ../../libs/virtualMachine/test/supercontracts/long_run.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/long_run.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest(CallRequestParameters{
                ContractKey(),
                CallId(),
                "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                20000000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        BlockHash(),
                        0,
                        0,
                        {}
                }
        }, CallRequest::CallLevel::MANUAL);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_TRUE(res);
            // std::cout << res->m_success << std::endl;
            // std::cout << res->m_return << std::endl;
            // std::cout << res->m_scConsumed << std::endl;
            // std::cout << res->m_smConsumed << std::endl;
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_scConsumed, 0);
            ASSERT_EQ(res->m_smConsumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    // pVirtualMachine.reset();
    threadManager.execute([&] { pVirtualMachine.reset(); });

    barrier.get();

    threadManager.stop();
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, FaultyContract) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // The contract should panic in this case due to failing the assertion
        // exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read_faulty.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/internet_read_faulty.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest(CallRequestParameters{
                ContractKey(),
                CallId(),
                "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        BlockHash(),
                        0,
                        0,
                        {}
                }
        }, CallRequest::CallLevel::MANUAL);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_TRUE(res);
            // std::cout << res->m_success << std::endl;
            // std::cout << res->m_return << std::endl;
            // std::cout << res->m_scConsumed << std::endl;
            // std::cout << res->m_smConsumed << std::endl;
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_scConsumed, 20525636378);
            ASSERT_EQ(res->m_smConsumed, 10240);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    barrier.get();

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}


/**
Prerequisites: the user must be allowed to run sudo systemctl without password
*/
TEST(VirtualMachine, AbortServerDuringExecution) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> pAborted;
    auto barrierAborted = pAborted.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // exec("cp ../../libs/virtualMachine/test/supercontracts/long_run.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/long_run.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest(CallRequestParameters{
                ContractKey(),
                CallId(),
                "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                20000000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        BlockHash(),
                        0,
                        0,
                        {}
                }
        }, CallRequest::CallLevel::MANUAL);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {
            // TODO on call executed
            ASSERT_FALSE(res);
            pAborted.set_value();
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    exec("sudo systemctl stop supercontract_server");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    exec("sudo systemctl start supercontract_server");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    barrierAborted.get();

    std::promise<void> pSuccessful;
    auto barrierSuccesful = pSuccessful.get_future();

    threadManager.execute([&] {
        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest(CallRequestParameters{
                ContractKey(),
                CallId(),
                "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                20000000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        BlockHash(),
                        0,
                        0,
                        {}
                }
        }, CallRequest::CallLevel::MANUAL);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {
            // TODO on call executed
            pSuccessful.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    barrierSuccesful.wait();

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

} // namespace sirius::contract::vm::test