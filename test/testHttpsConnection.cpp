/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "gtest/gtest.h"
#include "supercontract/HttpsInternetConnection.h"

namespace sirius::contract::test {

#define TEST_NAME HttpsConnection

TEST(Example, TEST_NAME) {
    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ThreadManager threadManager;
    DebugInfo info{ "peer", threadManager.threadId() };
    auto connection =
            std::make_shared<HttpsInternetConnection>(ctx, threadManager, "example.com", "/", 30000, info);
    std::function<void(bool&&)> f = [] (bool&& s) {
        std::cout << "success " << s << std::endl;
    };
    std::function<void()> terminateCallback = [] {};
    auto query = std::make_shared<AbstractAsyncQuery<bool>>(f, terminateCallback, threadManager);
    threadManager.execute([=] {
        connection->open(query);
    });
    threadManager.stop();
}

}