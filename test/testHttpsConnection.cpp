/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "gtest/gtest.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/x509_vfy.h>
#include <openssl/ssl.h>
#include <openssl/ocsp.h>
#include <thread>

#include "supercontract/HttpsInternetConnection.h"


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

using std::cout;
using std::endl;
using std::stringstream;
using std::map;
using std::vector;
using std::string;

namespace sirius::contract::test {

#define TEST_NAME HttpsConnection

TEST(Example, TEST_NAME) {

    ThreadManager threadManager;

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode( ssl::verify_peer );

    DebugInfo info{ "peer", threadManager.threadId() };
    auto connection =
            std::make_shared<HttpsInternetConnection>(ctx, threadManager, "revoked.badssl.com", "/", 30000, info);

    std::function<void(bool&&)> f = [&] (bool&& s) {
        std::cout << "success " << s << std::endl;
    };
    std::function<void()> terminateCallback = [] {};
    auto query = std::make_shared<AbstractAsyncQuery<bool>>(f, terminateCallback, threadManager);

    threadManager.execute([&] {
        connection->open(query);
    });
    threadManager.stop();
    std::cout<< "stopped" << std::endl;
}

}