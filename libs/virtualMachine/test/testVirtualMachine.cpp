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
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest = {
            ContractKey(),
            CallId(),
            "../../../rust-xpx-supercontract-client-sdk/pkg/simple_contract.wasm",
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
            /*
                #[no_mangle]
                pub unsafe extern "C" fn run() -> u32 {
                    return 1 + 1;
                }
            */
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1 + 1);
            ASSERT_EQ(res->m_scConsumed, 92);
            ASSERT_EQ(res->m_smConsumed, 0);
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, std::weak_ptr<VirtualMachineInternetQueryHandler>(),
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), callback); });

        barrier.get();

        threadManager.execute([&]
                              { pVirtualMachine.reset(); });

        threadManager.stop();
    }

    TEST(VirtualMachine, InternetRead)
    {

        GlobalEnvironmentMock environment;
        auto &threadManager = environment.threadManager();

        auto storageObserver = std::make_shared<StorageContentManagerMock>();

        std::shared_ptr<VirtualMachine> pVirtualMachine;

        std::promise<void> p;
        auto barrier = p.get_future();

        threadManager.execute([&]
                              {
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        CallRequest callRequest = {
            ContractKey(),
            CallId(),
            "../../../rust-xpx-supercontract-client-sdk/pkg/internet_read.wasm",
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

        auto internetHandler = std::make_shared<MockVirtualMachineInternetQueryHandler>();

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
    }

}