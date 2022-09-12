/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "gtest/gtest.h"
#include <boost/beast/ssl.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "internet/InternetConnection.h"
#include "internet/InternetUtils.h"
#include "TestUtils.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace sirius::contract::internet::test {

#define TEST_NAME = HttpsConnection

TEST(HttpsConnection, ValidRead) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode( ssl::verify_peer );

    auto urlDescription = parseURL( "https://example.com" );

    ASSERT_TRUE( urlDescription );
    ASSERT_TRUE( urlDescription->ssl );
    ASSERT_EQ( urlDescription->port, "443" );

    auto connectionCallback = createAsyncQueryHandler<std::optional<InternetConnection>>(
            [&]( std::optional<InternetConnection>&& connection ) {
                ASSERT_TRUE( connection );

                auto sharedConnection = std::make_shared<InternetConnection>(std::move(*connection));

                auto readCallback = createAsyncQueryHandler<std::optional<std::vector<uint8_t>>>([connection = sharedConnection] (std::optional<std::vector<uint8_t>>&& res) {
                    ASSERT_TRUE( res.has_value() );
                    std::string actual(res->begin(), res->end());
                    const std::string expected = "<!doctype html>\n"
                                                 "<html>\n"
                                                 "<head>\n"
                                                 "    <title>Example Domain</title>\n"
                                                 "\n"
                                                 "    <meta charset=\"utf-8\" />\n"
                                                 "    <meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\" />\n"
                                                 "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n"
                                                 "    <style type=\"text/css\">\n"
                                                 "    body {\n"
                                                 "        background-color: #f0f0f2;\n"
                                                 "        margin: 0;\n"
                                                 "        padding: 0;\n"
                                                 "        font-family: -apple-system, system-ui, BlinkMacSystemFont, \"Segoe UI\", \"Open Sans\", \"Helvetica Neue\", Helvetica, Arial, sans-serif;\n"
                                                 "        \n"
                                                 "    }\n"
                                                 "    div {\n"
                                                 "        width: 600px;\n"
                                                 "        margin: 5em auto;\n"
                                                 "        padding: 2em;\n"
                                                 "        background-color: #fdfdff;\n"
                                                 "        border-radius: 0.5em;\n"
                                                 "        box-shadow: 2px 3px 7px 2px rgba(0,0,0,0.02);\n"
                                                 "    }\n"
                                                 "    a:link, a:visited {\n"
                                                 "        color: #38488f;\n"
                                                 "        text-decoration: none;\n"
                                                 "    }\n"
                                                 "    @media (max-width: 700px) {\n"
                                                 "        div {\n"
                                                 "            margin: 0 auto;\n"
                                                 "            width: auto;\n"
                                                 "        }\n"
                                                 "    }\n"
                                                 "    </style>    \n"
                                                 "</head>\n"
                                                 "\n"
                                                 "<body>\n"
                                                 "<div>\n"
                                                 "    <h1>Example Domain</h1>\n"
                                                 "    <p>This domain is for use in illustrative examples in documents. You may use this\n"
                                                 "    domain in literature without prior coordination or asking for permission.</p>\n"
                                                 "    <p><a href=\"https://www.iana.org/domains/example\">More information...</a></p>\n"
                                                 "</div>\n"
                                                 "</body>\n"
                                                 "</html>\n";
                    ASSERT_EQ( actual, expected );
                    }, [] {}, globalEnvironment);
                sharedConnection->read(readCallback);
            },
            [] {},
            globalEnvironment );

    threadManager.execute([&] {
        InternetConnection::buildHttpsInternetConnection(ctx,
                                                         globalEnvironment,
                                                         urlDescription->host,
                                                         urlDescription->port,
                                                         urlDescription->target,
                                                         16 * 1024,
                                                         30000,
                                                         500,
                                                         60,
                                                         RevocationVerificationMode::HARD,
                                                         connectionCallback );
    });
    threadManager.stop();
}

TEST(HttpsConnection, ValidCertificate) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode( ssl::verify_peer );

    auto urlDescription = parseURL( "https://example.com" );

    ASSERT_TRUE( urlDescription );
    ASSERT_TRUE( urlDescription->ssl );
    ASSERT_EQ( urlDescription->port, "443" );

    auto connectionCallback = createAsyncQueryHandler<std::optional<InternetConnection>>(
            [&]( std::optional<InternetConnection>&& connection ) {
                ASSERT_TRUE( connection );
            }, [] {}, globalEnvironment );

    threadManager.execute([&] {
        InternetConnection::buildHttpsInternetConnection(ctx,
                                                         globalEnvironment,
                                                         urlDescription->host,
                                                         urlDescription->port,
                                                         urlDescription->target,
                                                         16 * 1024,
                                                         30000,
                                                         500,
                                                         60,
                                                         RevocationVerificationMode::HARD,
                                                         connectionCallback );
    });
    threadManager.stop();
}

TEST(HttpsConnection, TerminateCall) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode( ssl::verify_peer );

    auto urlDescription = parseURL( "https://example.com" );

    ASSERT_TRUE( urlDescription );
    ASSERT_TRUE( urlDescription->ssl );
    ASSERT_EQ( urlDescription->port, "443" );

    bool flag = false;

    auto connectionCallback = createAsyncQueryHandler<std::optional<InternetConnection>>(
            [&]( std::optional<InternetConnection>&& connection ) {
                flag = true;
                ASSERT_TRUE( connection );
            }, [] {}, globalEnvironment );

    threadManager.execute([&] {
        InternetConnection::buildHttpsInternetConnection(ctx,
                                                         globalEnvironment,
                                                         urlDescription->host,
                                                         urlDescription->port,
                                                         urlDescription->target,
                                                         16 * 1024,
                                                         30000,
                                                         500,
                                                         60,
                                                         RevocationVerificationMode::HARD,
                                                         connectionCallback );
    });
    connectionCallback->terminate();
    threadManager.stop();
    ASSERT_FALSE(flag);
}

TEST(HttpsConnection, RevokedCertificate) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode( ssl::verify_peer );

    auto urlDescription = parseURL( "https://revoked.badssl.com/" );

    ASSERT_TRUE( urlDescription );
    ASSERT_TRUE( urlDescription->ssl );
    ASSERT_EQ( urlDescription->port, "443" );

    auto connectionCallback = createAsyncQueryHandler<std::optional<InternetConnection>>(
            [&]( std::optional<InternetConnection>&& connection ) {
                ASSERT_TRUE( !connection );
                }, [] {}, globalEnvironment );

    threadManager.execute([&] {
        InternetConnection::buildHttpsInternetConnection(ctx,
                                                         globalEnvironment,
                                                         urlDescription->host,
                                                         urlDescription->port,
                                                         urlDescription->target,
                                                         16 * 1024,
                                                         30000,
                                                         500,
                                                         60,
                                                         RevocationVerificationMode::HARD,
                                                         connectionCallback );
    });
    threadManager.stop();
}

}