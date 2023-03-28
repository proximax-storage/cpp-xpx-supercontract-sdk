/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "MockInternetHandler.h"
#include "FaultyMockStorageHandler.h"
#include "MockStorageHandler.h"
#include <storage/StorageErrorCode.h>
#include "TestUtils.h"
#include <gtest/gtest.h>
#include <virtualMachine/RPCVirtualMachineBuilder.h>
#include <virtualMachine/ExecutionErrorConidition.h>
#include <boost/process.hpp>

namespace sirius::contract::vm::test {

namespace {

void exec(const char* cmd) {
    boost::process::child c(cmd);
    c.join();
    ASSERT_EQ(c.exit_code(), 0);
}

std::optional<std::string> vmAddress() {
#ifdef SIRIUS_CONTRACT_VM_ADDRESS_TEST
    return  SIRIUS_CONTRACT_VM_ADDRESS_TEST;
#else
    return {};
#endif
};

}

TEST(VirtualMachine, SimpleContract) {

    auto virtualMachineAddressOpt = vmAddress();

    if (!virtualMachineAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy("supercontracts/simple.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");

    threadManager.execute([&] {
        RPCVirtualMachineBuilder builder(*virtualMachineAddressOpt);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);
        
        std::vector<uint8_t> params;
        vm::CallRequest callRequest = CallRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                52000000,
                20 * 1024,
                CallRequest::CallLevel::AUTOMATIC,
                0);

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1 + 1);
            ASSERT_EQ(res->m_execution_gas_consumed, 604);
            ASSERT_EQ(res->m_download_gas_consumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, std::weak_ptr<VirtualMachineInternetQueryHandler>(),
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(),
                                     std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, InternetRead) {

    auto virtualMachineAddressOpt = vmAddress();

    if (!virtualMachineAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy("supercontracts/internet_read.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");
    threadManager.execute([&] {
        RPCVirtualMachineBuilder builder(*virtualMachineAddressOpt);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);


        std::vector<uint8_t> params;
        CallRequest callRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallRequest::CallLevel::MANUAL,
                0);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1);
            ASSERT_EQ(res->m_execution_gas_consumed, 20577370024);
            ASSERT_EQ(res->m_download_gas_consumed, 10240);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(),
                                     std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, InternetReadNotEnoughSC) {

    auto virtualMachineAddressOpt = vmAddress();

    if (!virtualMachineAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy("supercontracts/internet_read.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");

    threadManager.execute([&] {
        RPCVirtualMachineBuilder builder(*virtualMachineAddressOpt);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);


        std::vector<uint8_t> params;
        CallRequest callRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                100000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallRequest::CallLevel::MANUAL,
                0);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_execution_gas_consumed, 100000);
            ASSERT_EQ(res->m_download_gas_consumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(),
                                     std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, InternetReadNotEnoughSM) {

    auto virtualMachineAddressOpt = vmAddress();

    if (!virtualMachineAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy("supercontracts/internet_read.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");
    threadManager.execute([&] {
        RPCVirtualMachineBuilder builder(*virtualMachineAddressOpt);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);


        std::vector<uint8_t> params;
        CallRequest callRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                25 * 1024,
                CallRequest::CallLevel::MANUAL,
                0);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_execution_gas_consumed, 1483935301);
            ASSERT_EQ(res->m_download_gas_consumed, 10 * 1024);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(),
                                     std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, WrongContractPath) {

    auto virtualMachineAddressOpt = vmAddress();

    if (!virtualMachineAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy("supercontracts/internet_read.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");

    threadManager.execute([&] {
        RPCVirtualMachineBuilder builder(*virtualMachineAddressOpt);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);


        std::vector<uint8_t> params;
        CallRequest callRequest(
                CallId(),
                "sdk_bg.wasm",
                "run",
                params,
                25000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallRequest::CallLevel::MANUAL, 0);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_execution_gas_consumed, 0);
            ASSERT_EQ(res->m_download_gas_consumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(),
                                     std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
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
    std::filesystem::copy("supercontracts/internet_read.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");
    threadManager.execute([&] {
        std::string address = "127.0.0.1:50052";
        RPCVirtualMachineBuilder builder(address);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);


        std::vector<uint8_t> params;
        CallRequest callRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallRequest::CallLevel::MANUAL, 0);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            p.set_value();
            ASSERT_FALSE(res);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(),
                                     std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, WrongExecFunction) {

    auto virtualMachineAddressOpt = vmAddress();

    if (!virtualMachineAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy("supercontracts/internet_read.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");
    threadManager.execute([&] {
        RPCVirtualMachineBuilder builder(*virtualMachineAddressOpt);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);


        std::vector<uint8_t> params;
        CallRequest callRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "runs",
                params,
                25000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallRequest::CallLevel::MANUAL, 0);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_execution_gas_consumed, 0);
            ASSERT_EQ(res->m_download_gas_consumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(),
                                     std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, UnauthorizedImportFunction) {

    auto virtualMachineAddressOpt = vmAddress();

    if (!virtualMachineAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy("supercontracts/internet_read.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");

    threadManager.execute([&] {
        RPCVirtualMachineBuilder builder(*virtualMachineAddressOpt);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);


        std::vector<uint8_t> params;
        CallRequest callRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallRequest::CallLevel::AUTOMATIC,
                0);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_execution_gas_consumed, 532447);
            ASSERT_EQ(res->m_download_gas_consumed, 0); // Internet read function shouldn't be called, so no SM is consumed
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(),
                                     std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, AbortVMDuringExecution) {

    auto virtualMachineAddressOpt = vmAddress();

    if (!virtualMachineAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy("supercontracts/long_run.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");

    threadManager.execute([&] {
        RPCVirtualMachineBuilder builder(*virtualMachineAddressOpt);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);


        std::vector<uint8_t> params;
        CallRequest callRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                20000000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallRequest::CallLevel::MANUAL, 0);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            p.set_value();
            ASSERT_FALSE(res);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(),
                                     std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    // pVirtualMachine.reset();
    threadManager.execute([&] { pVirtualMachine.reset(); });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, FaultyContract) {

    auto virtualMachineAddressOpt = vmAddress();

    if (!virtualMachineAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    // The contract should panic in this case due to failing the assertion
    std::filesystem::copy("supercontracts/internet_read_faulty.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");

    threadManager.execute([&] {
        RPCVirtualMachineBuilder builder(*virtualMachineAddressOpt);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);


        std::vector<uint8_t> params;
        CallRequest callRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallRequest::CallLevel::MANUAL, 0);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_execution_gas_consumed, 20524893109);
            ASSERT_EQ(res->m_download_gas_consumed, 10240);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(),
                                     std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}


/**
Prerequisites: the user must be allowed to run sudo systemctl without password
*/
TEST(VirtualMachine, AbortServerDuringExecution) {

#ifndef SIRIUS_CONTRACT_RUN_SUDO_TESTS
    GTEST_SKIP();
#endif

    auto virtualMachineAddressOpt = vmAddress();

    if (!virtualMachineAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    std::promise<void> pAborted;
    auto barrierAborted = pAborted.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy("supercontracts/long_run.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");

    threadManager.execute([&] {
        RPCVirtualMachineBuilder builder(*virtualMachineAddressOpt);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);


        std::vector<uint8_t> params;
        CallRequest callRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                20000000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallRequest::CallLevel::MANUAL, 0);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            ASSERT_FALSE(res);
            pAborted.set_value();
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(),
                                     std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    exec("sudo systemctl stop supercontract_server");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    exec("sudo systemctl start supercontract_server");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    ASSERT_EQ(std::future_status::ready, barrierAborted.wait_for(std::chrono::seconds(10)));

    std::promise<void> pSuccessful;
    auto barrierSuccessful = pSuccessful.get_future();

    threadManager.execute([&] {
        std::vector<uint8_t> params;
        CallRequest callRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                20000000000000,
                // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
                // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
                26 * 1024,
                CallRequest::CallLevel::MANUAL, 0);

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            pSuccessful.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(),
                                     std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    ASSERT_EQ(std::future_status::ready, barrierSuccessful.wait_for(std::chrono::minutes(3)));

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, SimpleStorage) {

    auto virtualMachineAddressOpt = vmAddress();

    if (!virtualMachineAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockStorageHandler> storageHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy("supercontracts/storage.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");

    threadManager.execute([&] {
        RPCVirtualMachineBuilder builder(*virtualMachineAddressOpt);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);


        std::vector<uint8_t> params;
        vm::CallRequest callRequest = CallRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                26 * 1024,
                CallRequest::CallLevel::MANUAL, 0);

        storageHandler = std::make_shared<MockStorageHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1);
            ASSERT_EQ(res->m_execution_gas_consumed, 4402891458);
            ASSERT_EQ(res->m_download_gas_consumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, std::weak_ptr<VirtualMachineInternetQueryHandler>(),
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), storageHandler, callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, IteratorTest) {

    auto virtualMachineAddressOpt = vmAddress();

    if (!virtualMachineAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<MockStorageHandler> storageHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy("supercontracts/iterator.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");

    threadManager.execute([&] {
        RPCVirtualMachineBuilder builder(*virtualMachineAddressOpt);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);


        std::vector<uint8_t> params;
        vm::CallRequest callRequest = CallRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                26 * 1024,
                CallRequest::CallLevel::MANUAL, 0);

        storageHandler = std::make_shared<MockStorageHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1);
            ASSERT_EQ(res->m_execution_gas_consumed, 218275185);
            ASSERT_EQ(res->m_download_gas_consumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, std::weak_ptr<VirtualMachineInternetQueryHandler>(),
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), storageHandler, callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, FaultyStorage) {

    auto virtualMachineAddressOpt = vmAddress();

    if (!virtualMachineAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::shared_ptr<FaultyMockStorageHandler> storageHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy("supercontracts/storage_faulty.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");

    threadManager.execute([&] {
        RPCVirtualMachineBuilder builder(*virtualMachineAddressOpt);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);


        std::vector<uint8_t> params;
        vm::CallRequest callRequest = CallRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                26 * 1024,
                CallRequest::CallLevel::MANUAL, 0);

        storageHandler = std::make_shared<FaultyMockStorageHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {
            p.set_value();
            ASSERT_FALSE(res);
            ASSERT_EQ(res.error(), ExecutionError::storage_unavailable);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, std::weak_ptr<VirtualMachineInternetQueryHandler>(),
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), storageHandler, callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

TEST(VirtualMachine, NullStorageHandler) {

    auto virtualMachineAddressOpt = vmAddress();

    if (!virtualMachineAddressOpt) {
        GTEST_SKIP();
    }

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy("supercontracts/storage.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs",
                          copyOptions);
    exec("wasm-pack build --debug rust-xpx-supercontract-client-sdk/");

    threadManager.execute([&] {
        RPCVirtualMachineBuilder builder(*virtualMachineAddressOpt);
        builder.setStorage(storageObserver);
        pVirtualMachine = builder.build(environment);


        std::vector<uint8_t> params;
        vm::CallRequest callRequest = CallRequest(
                CallId(),
                "rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                "run",
                params,
                25000000000,
                26 * 1024,
                CallRequest::CallLevel::MANUAL, 0);

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&](auto&& res) {

            p.set_value();
            ASSERT_FALSE(res);
            ASSERT_EQ(res.error(), ExecutionError::storage_unavailable);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, std::weak_ptr<VirtualMachineInternetQueryHandler>(),
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(),
                                     std::weak_ptr<VirtualMachineStorageQueryHandler>(), callback);
    });

    ASSERT_EQ(std::future_status::ready, barrier.wait_for(std::chrono::seconds(10)));

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    std::filesystem::copy("supercontracts/lib.rs",
                          "rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}

} // namespace sirius::contract::vm::test