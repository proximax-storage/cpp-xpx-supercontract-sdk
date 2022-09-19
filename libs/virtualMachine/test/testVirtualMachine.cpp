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

    StorageObserverMock storageObserver;

    threadManager.execute([&] {
        // TODO Fill in the address
        std::string address;
        RPCVirtualMachineBuilder builder;
        auto pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        CallRequest callRequest;

        auto[_, callback] = createAsyncQuery<std::optional<CallExecutionResult>>([] (auto&& res) {
            // TODO on call executed
        }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, std::weak_ptr<VirtualMachineInternetQueryHandler>(),
                                     std::weak_ptr<VirtualMachineBlockchainQueryHandler>(), callback);
    });

    threadManager.stop();
}

}