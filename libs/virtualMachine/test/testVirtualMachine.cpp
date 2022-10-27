/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <gtest/gtest.h>
#include <virtualMachine/RPCVirtualMachineBuilder.h>
#include "TestUtils.h"

namespace sirius::contract::vm::test {

TEST(VirtualMachine, Example) {

    GlobalEnvironmentMock environment;
    auto& threadManager = environment.threadManager();

    auto storageObserver = std::make_shared<StorageContentManagerMock>();

    std::shared_ptr<VirtualMachine> pVirtualMachine;

    std::promise<void> p;
    auto barrier = p.get_future();

    threadManager.execute([&] {
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        vm::CallRequest callRequest = CallRequest(CallRequestParameters{
                ContractKey(),
                CallId(),
                "../../../rust-xpx-supercontract-client-sdk/pkg/simple_contract.wasm",
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
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, std::weak_ptr<VirtualMachineInternetQueryHandler>(),
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), callback);
    });

    barrier.get();

    threadManager.execute([&] {
        pVirtualMachine.reset();
    });

    threadManager.stop();
}

}