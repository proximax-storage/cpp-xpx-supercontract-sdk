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
                auto readCallback = createAsyncQueryHandler<std::optional<std::vector<uint8_t>>>([connection = std::move(*connection)] (std::optional<std::vector<uint8_t>>&& res) {
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
                    // connection->getContainer()->read(readCallback);
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

TEST(HttpsConnection, NonExisting)
{

    GlobalEnvironmentImpl globalEnvironment;
    auto &threadManager = globalEnvironment.threadManager();

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode(ssl::verify_peer);

    auto urlDescription = parseURL("https://examples123.com");

    ASSERT_TRUE( urlDescription );
    ASSERT_TRUE( urlDescription->ssl );
    ASSERT_EQ( urlDescription->port, "443" );

    auto connectionCallback = createAsyncQueryHandler<std::optional<InternetConnection>>(
        [&](std::optional<InternetConnection> &&connection)
        {
            ASSERT_FALSE(connection);
        },
        [] {}, globalEnvironment);

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

TEST(HttpsConnection, NonExistingTarget)
{

    GlobalEnvironmentImpl globalEnvironment;
    auto &threadManager = globalEnvironment.threadManager();

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode(ssl::verify_peer);

    auto urlDescription = parseURL("https://www.google.com/eg");

    ASSERT_TRUE(urlDescription);
    ASSERT_TRUE(urlDescription->ssl);
    ASSERT_EQ(urlDescription->port, "443");

        auto connectionCallback = createAsyncQueryHandler<std::optional<InternetConnection>>(
            [&](std::optional<InternetConnection> &&connection)
            {
                ASSERT_TRUE(connection);
                auto readCallback = createAsyncQueryHandler<std::optional<std::vector<uint8_t>>>([connection = std::move(*connection)](std::optional<std::vector<uint8_t>> &&res)
                                                                                                 {
                    ASSERT_TRUE( res.has_value() );
                    std::string actual(res->begin(), res->end());
                    const std::string expected = "<!DOCTYPE html>\n"
                                                "<!-- saved from url=(0014)about:internet -->\n"
                                                "<html lang=\"en\"><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n"
                                                "  <meta name=\"viewport\" content=\"initial-scale=1, minimum-scale=1, width=device-width\">\n"
                                                "  <title>Error 404 (Not Found)!!1</title>\n"
                                                "  <style>\n"
                                                "    *{margin:0;padding:0}html,code{font:15px/22px arial,sans-serif}html{background:#fff;color:#222;padding:15px}body{margin:7% auto 0;max-width:390px;min-height:180px;padding:30px 0 15px}* > body{background:url(//www.google.com/images/errors/robot.png) 100% 5px no-repeat;padding-right:205px}p{margin:11px 0 22px;overflow:hidden}ins{color:#777;text-decoration:none}a img{border:0}@media screen and (max-width:772px){body{background:none;margin-top:0;max-width:none;padding-right:0}}#logo{background:url(//www.google.com/images/branding/googlelogo/1x/googlelogo_color_150x54dp.png) no-repeat;margin-left:-5px}@media only screen and (min-resolution:192dpi){#logo{background:url(//www.google.com/images/branding/googlelogo/2x/googlelogo_color_150x54dp.png) no-repeat 0% 0%/100% 100%;-moz-border-image:url(//www.google.com/images/branding/googlelogo/2x/googlelogo_color_150x54dp.png) 0}}@media only screen and (-webkit-min-device-pixel-ratio:2){#logo{background:url(//www.google.com/images/branding/googlelogo/2x/googlelogo_color_150x54dp.png) no-repeat;-webkit-background-size:100% 100%}}#logo{display:inline-block;height:54px;width:150px}\n"
                                                "  </style>\n"
                                                "  </head><body><a href=\"https://www.google.com/\"><span id=\"logo\" aria-label=\"Google\"></span></a>\n"
                                                "  <p><b>404.</b> <ins>That's an error.</ins>\n"
                                                "  </p><p>The requested URL <code>/signin</code> was not found on this server.  <ins>That's all we know.</ins>\n"
                                                "</p></body></html>";
                    ASSERT_EQ( actual, expected ); },
                                                                                                 [] {}, globalEnvironment);
                connection->getContainer()->read(readCallback);
            },
            [] {}, globalEnvironment);

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

TEST(HttpsConnection, ExpiredCertificate) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode( ssl::verify_peer );

    auto urlDescription = parseURL( "https://expired.badssl.com/" );

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

TEST(HttpsConnection, UntrustedRootCertificate) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode( ssl::verify_peer );

    auto urlDescription = parseURL( "https://untrusted-root.badssl.com/" );

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

TEST(HttpsConnection, SelfSignedCertificate) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode( ssl::verify_peer );

    auto urlDescription = parseURL( "https://self-signed.badssl.com/" );

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

TEST(HttpsConnection, WrongHostCertificate) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode( ssl::verify_peer );

    auto urlDescription = parseURL( "https://wrong.host.badssl.com/" );

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

TEST(HttpsConnection, PinningTest) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode( ssl::verify_peer );

    auto urlDescription = parseURL( "https://pinning-test.badssl.com/" );

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

TEST(HttpsConnection, ClientCertMissing) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode( ssl::verify_peer );

    auto urlDescription = parseURL( "https://client-cert-missing.badssl.com/" );

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

TEST(HttpsConnection, WeakSignature) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode( ssl::verify_peer );

    auto urlDescription = parseURL( "https://sha1-2017.badssl.com/" );

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