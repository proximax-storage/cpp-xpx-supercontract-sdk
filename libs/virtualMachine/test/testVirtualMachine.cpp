/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <gtest/gtest.h>
#include <virtualMachine/RPCVirtualMachineBuilder.h>
#include "TestUtils.h"
#include "MockInternetHandler.h"

namespace sirius::contract::vm::test
{
    // https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
    std::string exec(const char *cmd)
    {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe)
        {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            result += buffer.data();
        }
        return result;
    }

    TEST(VirtualMachine, SimpleContract)
    {

        GlobalEnvironmentMock environment;
        auto &threadManager = environment.threadManager();

        auto storageObserver = std::make_shared<StorageContentManagerMock>();

        std::shared_ptr<VirtualMachine> pVirtualMachine;

        std::promise<void> p;
        auto barrier = p.get_future();

        threadManager.execute([&]
                              {
        exec("cp ../../libs/virtualMachine/test/supercontracts/simple.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest = {
            ContractKey(),
            CallId(),
            "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
            "run",
            params,
            52000000,
            20 * 1024,
            CallRequest::CallLevel::AUTOMATIC,
            CallReferenceInfo {
                {},
                0,
                BlockHash(),
                0,
                0,
                {}
            }
        };

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&] (auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1 + 1);
            ASSERT_EQ(res->m_scConsumed, 604);
            ASSERT_EQ(res->m_smConsumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, std::weak_ptr<VirtualMachineInternetQueryHandler>(),
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), callback); });

        barrier.get();

        threadManager.execute([&]
                              { pVirtualMachine.reset(); });

        threadManager.stop();
        exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    }

    TEST(VirtualMachine, InternetRead)
    {

        GlobalEnvironmentMock environment;
        auto &threadManager = environment.threadManager();

        auto storageObserver = std::make_shared<StorageContentManagerMock>();

        std::shared_ptr<VirtualMachine> pVirtualMachine;

        std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

        std::promise<void> p;
        auto barrier = p.get_future();

        threadManager.execute([&]
                              {
        exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest = {
            ContractKey(),
            CallId(),
            "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
            "run",
            params,
            25000000000,
            // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
            // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
            26 * 1024,
            CallRequest::CallLevel::MANUAL,
            CallReferenceInfo {
                {},
                0,
                BlockHash(),
                0,
                0,
                {}
            }
        };

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&] (auto&& res) {
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
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), callback); });

        barrier.get();

        threadManager.execute([&]
                              { pVirtualMachine.reset(); });

        threadManager.stop();
        exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    }

    TEST(VirtualMachine, InternetReadNotEnoughSC)
    {

        GlobalEnvironmentMock environment;
        auto &threadManager = environment.threadManager();

        auto storageObserver = std::make_shared<StorageContentManagerMock>();

        std::shared_ptr<VirtualMachine> pVirtualMachine;

        std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

        std::promise<void> p;
        auto barrier = p.get_future();

        threadManager.execute([&]
                              {
        exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest = {
            ContractKey(),
            CallId(),
            "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
            "run",
            params,
            100000,
            // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
            // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
            26 * 1024,
            CallRequest::CallLevel::MANUAL,
            CallReferenceInfo {
                {},
                0,
                BlockHash(),
                0,
                0,
                {}
            }
        };

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&] (auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_scConsumed, 100000);
            ASSERT_EQ(res->m_smConsumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), callback); });

        barrier.get();

        threadManager.execute([&]
                              { pVirtualMachine.reset(); });

        threadManager.stop();
        exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    }

    TEST(VirtualMachine, InternetReadNotEnoughSM)
    {

        GlobalEnvironmentMock environment;
        auto &threadManager = environment.threadManager();

        auto storageObserver = std::make_shared<StorageContentManagerMock>();

        std::shared_ptr<VirtualMachine> pVirtualMachine;

        std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

        std::promise<void> p;
        auto barrier = p.get_future();

        threadManager.execute([&]
                              {
        exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest = {
            ContractKey(),
            CallId(),
            "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
            "run",
            params,
            25000000000,
            // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
            // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
            25 * 1024,
            CallRequest::CallLevel::MANUAL,
            CallReferenceInfo {
                {},
                0,
                BlockHash(),
                0,
                0,
                {}
            }
        };

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&] (auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_scConsumed, 1484710915);
            ASSERT_EQ(res->m_smConsumed, 10 * 1024);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), callback); });

        barrier.get();

        threadManager.execute([&]
                              { pVirtualMachine.reset(); });

        threadManager.stop();
        exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    }

    TEST(VirtualMachine, WrongContractPath)
    {

        GlobalEnvironmentMock environment;
        auto &threadManager = environment.threadManager();

        auto storageObserver = std::make_shared<StorageContentManagerMock>();

        std::shared_ptr<VirtualMachine> pVirtualMachine;

        std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

        std::promise<void> p;
        auto barrier = p.get_future();

        threadManager.execute([&]
                              {
        exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest = {
            ContractKey(),
            CallId(),
            "sdk_bg.wasm",
            "run",
            params,
            25000000000,
            // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
            // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
            26 * 1024,
            CallRequest::CallLevel::MANUAL,
            CallReferenceInfo {
                {},
                0,
                BlockHash(),
                0,
                0,
                {}
            }
        };

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&] (auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_FALSE(res);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), callback); });

        barrier.get();

        threadManager.execute([&]
                              { pVirtualMachine.reset(); });

        threadManager.stop();
        exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    }

    TEST(VirtualMachine, WrongIP)
    {

        GlobalEnvironmentMock environment;
        auto &threadManager = environment.threadManager();

        auto storageObserver = std::make_shared<StorageContentManagerMock>();

        std::shared_ptr<VirtualMachine> pVirtualMachine;

        std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

        std::promise<void> p;
        auto barrier = p.get_future();

        threadManager.execute([&]
                              {
        exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50052";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest = {
            ContractKey(),
            CallId(),
            "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
            "run",
            params,
            25000000000,
            // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
            // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
            26 * 1024,
            CallRequest::CallLevel::MANUAL,
            CallReferenceInfo {
                {},
                0,
                BlockHash(),
                0,
                0,
                {}
            }
        };

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&] (auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_FALSE(res);
            ASSERT_TRUE(res.error() == std::make_error_code(std::errc::connection_aborted));
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), callback); });

        barrier.get();

        threadManager.execute([&]
                              { pVirtualMachine.reset(); });

        threadManager.stop();
        exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    }

    TEST(VirtualMachine, WrongExecFunction)
    {

        GlobalEnvironmentMock environment;
        auto &threadManager = environment.threadManager();

        auto storageObserver = std::make_shared<StorageContentManagerMock>();

        std::shared_ptr<VirtualMachine> pVirtualMachine;

        std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

        std::promise<void> p;
        auto barrier = p.get_future();

        threadManager.execute([&]
                              {
        exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest = {
            ContractKey(),
            CallId(),
            "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
            "runs",
            params,
            25000000000,
            // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
            // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
            26 * 1024,
            CallRequest::CallLevel::MANUAL,
            CallReferenceInfo {
                {},
                0,
                BlockHash(),
                0,
                0,
                {}
            }
        };

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&] (auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_scConsumed, 0);
            ASSERT_EQ(res->m_smConsumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), callback); });

        barrier.get();

        threadManager.execute([&]
                              { pVirtualMachine.reset(); });

        threadManager.stop();
        exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    }

    TEST(VirtualMachine, UnauthorizedImportFunction)
    {

        GlobalEnvironmentMock environment;
        auto &threadManager = environment.threadManager();

        auto storageObserver = std::make_shared<StorageContentManagerMock>();

        std::shared_ptr<VirtualMachine> pVirtualMachine;

        std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

        std::promise<void> p;
        auto barrier = p.get_future();

        threadManager.execute([&]
                              {
        exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest = {
            ContractKey(),
            CallId(),
            "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
            "run",
            params,
            25000000000,
            // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
            // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
            26 * 1024,
            CallRequest::CallLevel::AUTOMATIC,
            CallReferenceInfo {
                {},
                0,
                BlockHash(),
                0,
                0,
                {}
            }
        };

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&] (auto&& res) {
            // TODO on call executed
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, false);
            ASSERT_EQ(res->m_return, 0);
            ASSERT_EQ(res->m_scConsumed, 1096579);
            ASSERT_EQ(res->m_smConsumed, 0); // Internet read function shouldn't be called, so no SM is consumed
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, internetHandler,
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), callback); });

        barrier.get();

        threadManager.execute([&]
                              { pVirtualMachine.reset(); });

        threadManager.stop();
        exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    }

    TEST(VirtualMachine, AbortVMDuringExecution)
    {

        GlobalEnvironmentMock environment;
        auto &threadManager = environment.threadManager();

        auto storageObserver = std::make_shared<StorageContentManagerMock>();

        std::shared_ptr<VirtualMachine> pVirtualMachine;

        std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

        std::promise<void> p;
        auto barrier = p.get_future();

        threadManager.execute([&]
                              {
        exec("cp ../../libs/virtualMachine/test/supercontracts/long_run.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest = {
            ContractKey(),
            CallId(),
            "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
            "run",
            params,
            20000000000000,
            // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
            // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
            26 * 1024,
            CallRequest::CallLevel::MANUAL,
            CallReferenceInfo {
                {},
                0,
                BlockHash(),
                0,
                0,
                {}
            }
        };

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&] (auto&& res) {
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
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), callback); });

        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        // pVirtualMachine.reset();
        threadManager.execute([&]
                              { pVirtualMachine.reset(); });

        barrier.get();

        threadManager.stop();
        exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    }

    TEST(VirtualMachine, FaultyContract)
    {

        GlobalEnvironmentMock environment;
        auto &threadManager = environment.threadManager();

        auto storageObserver = std::make_shared<StorageContentManagerMock>();

        std::shared_ptr<VirtualMachine> pVirtualMachine;

        std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

        std::promise<void> p;
        auto barrier = p.get_future();

        threadManager.execute([&]
                              {
        // The contract should panic in this case due to failing the assertion
        exec("cp ../../libs/virtualMachine/test/supercontracts/internet_read_faulty.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest = {
            ContractKey(),
            CallId(),
            "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
            "run",
            params,
            25000000000,
            // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
            // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
            26 * 1024,
            CallRequest::CallLevel::MANUAL,
            CallReferenceInfo {
                {},
                0,
                BlockHash(),
                0,
                0,
                {}
            }
        };

        internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

        auto[_, callback] = createAsyncQuery<CallExecutionResult>([&] (auto&& res) {
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
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), callback); });

        barrier.get();

        threadManager.execute([&]
                              { pVirtualMachine.reset(); });

        threadManager.stop();
        exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    }

    // TEST(VirtualMachine, BlockConnection)
    // {

    //     GlobalEnvironmentMock environment;
    //     auto &threadManager = environment.threadManager();

    //     auto storageObserver = std::make_shared<StorageContentManagerMock>();

    //     std::shared_ptr<VirtualMachine> pVirtualMachine;

    //     std::shared_ptr<MockVirtualMachineInternetQueryHandler> internetHandler;

    //     std::promise<void> p;
    //     auto barrier = p.get_future();

    //     threadManager.execute([&]
    //                           {
    //     exec("cp ../../libs/virtualMachine/test/supercontracts/long_run.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    //     exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
    //     // TODO Fill in the address
    //     std::string address = "127.0.0.1:50051";
    //     RPCVirtualMachineBuilder builder;
    //     pVirtualMachine = builder.build(storageObserver, environment, address);

    //     // TODO fill in the callRequest fields
    //     std::vector<uint8_t> params;
    //     CallRequest callRequest = {
    //         ContractKey(),
    //         CallId(),
    //         "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
    //         "run",
    //         params,
    //         20000000000000,
    //         // It needs to be 16kb more than the actual amount needed to pass the memory read limit check
    //         // (the last call to read rpc function should expect 0 return but it will need to pass the 16kb check first anyways)
    //         26 * 1024,
    //         CallRequest::CallLevel::MANUAL,
    //         CallReferenceInfo {
    //             {},
    //             0,
    //             BlockHash(),
    //             0,
    //             0,
    //             {}
    //         }
    //     };

    //     internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

    //     auto[_, callback] = createAsyncQuery<CallExecutionResult>([&] (auto&& res) {
    //         // TODO on call executed
    //         p.set_value();
    //         ASSERT_TRUE(res);
    //         // std::cout << res->m_success << std::endl;
    //         // std::cout << res->m_return << std::endl;
    //         // std::cout << res->m_scConsumed << std::endl;
    //         // std::cout << res->m_smConsumed << std::endl;
    //         ASSERT_EQ(res->m_success, false);
    //         ASSERT_EQ(res->m_return, 0);
    //         ASSERT_EQ(res->m_scConsumed, 20525636378);
    //         ASSERT_EQ(res->m_smConsumed, 10240);
    //     }, [] {}, environment, false, false);

    //     pVirtualMachine->executeCall(callRequest, internetHandler,
    //                                  std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), callback); });

    //     std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    //     threadManager.execute([&]
    //                           { exec("sudo iptables -A INPUT -p tcp --dport 50051 -s 127.0.0.1 -j DROP"); });

    //     barrier.get();

    //     threadManager.execute([&]
    //                           { pVirtualMachine.reset(); });

    //     threadManager.stop();
    //     exec("sudo iptables -D INPUT 1");
    //     exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    // }

}