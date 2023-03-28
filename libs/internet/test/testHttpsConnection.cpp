/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "gtest/gtest.h"
#include <boost/beast/ssl.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <fstream>

#include "internet/InternetConnection.h"
#include "internet/InternetUtils.h"
#include "TestUtils.h"

namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>

namespace sirius::contract::internet::test {

// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
std::string exec_https(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

#define TEST_NAME = HttpsConnection

TEST(HttpsConnection, ValidRead) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    threadManager.execute([&] {

        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://example.com");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[connectionQuery, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_TRUE(connection);

                    auto sharedConnection = std::make_shared<InternetConnection>(std::move(*connection));

                    auto[_, readCallback] = createAsyncQuery<std::vector<uint8_t>>(
                            [connection = sharedConnection](auto&& res) {
                                ASSERT_TRUE(res.has_value());
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
                                ASSERT_EQ(actual, expected);
                            }, [] {}, globalEnvironment, false, false);
                    sharedConnection->read(readCallback);
                },
                [] {},
                globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
}

TEST(HttpsConnection, ConnectingLocalhost) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_default_verify_paths();
    ctx.set_verify_mode(ssl::verify_peer);

    threadManager.execute([&] {
        auto urlDescription = parseURL("https://localhost");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_FALSE(connection);
                },
                [] {}, globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
}

void readFuncNormally(expected<std::vector<uint8_t>>&& res, bool& read_flag, std::vector<uint8_t>& actual_vec,
                      std::shared_ptr<sirius::contract::internet::InternetConnection> sharedConnection,
                      GlobalEnvironmentImpl& globalEnvironment) {
    read_flag = true;
    ASSERT_TRUE(res);
    actual_vec.insert(actual_vec.end(), res->begin(), res->end());

    if (res->empty()) {
        std::string actual(actual_vec.begin(), actual_vec.end());
        std::string expected = "</html>";
        std::size_t found = actual.find(expected);
        if (found == std::string::npos) { throw found; }
        return;
    }

    auto[_, readCallback] = createAsyncQuery<std::vector<uint8_t>>([&, sharedConnection](auto&& res) {
                                                                       readFuncNormally(std::move(res), read_flag, actual_vec, sharedConnection, globalEnvironment);
                                                                   },
                                                                   [] {}, globalEnvironment, false, true);
    sharedConnection->read(readCallback);
}

TEST(HttpsConnection, ReadBigWebsite) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    std::vector<uint8_t> actual_vec;
    bool read_flag = false;

    threadManager.execute([&] {

        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://en.wikipedia.org/wiki/Byzantine_Empire");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_TRUE(connection);

                    auto sharedConnection = std::make_shared<InternetConnection>(std::move(*connection));

                    // std::this_thread::sleep_for(std::chrono::milliseconds(20000));
                    auto[_, readCallback] = createAsyncQuery<std::vector<uint8_t>>(
                            [&, sharedConnection](auto&& res) {
                                readFuncNormally(std::move(res), read_flag, actual_vec, sharedConnection,
                                                 globalEnvironment);
                            },
                            [] {}, globalEnvironment, false, true);

                    sharedConnection->read(readCallback);
                },
                [] {},
                globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
    ASSERT_TRUE(read_flag);
}

void readFuncDisconneted(expected<std::vector<uint8_t>>&& res, bool& read_flag, std::vector<uint8_t>& actual_vec,
                         std::shared_ptr<sirius::contract::internet::InternetConnection> sharedConnection,
                         GlobalEnvironmentImpl& globalEnvironment) {
    if (!res.has_value()) {
        read_flag = true;
        return;
    }
    actual_vec.insert(actual_vec.end(), res->begin(), res->end());
    // std::cout << actual_vec.data() << std::endl;

    if (res->empty()) {
        return;
    }

    auto[_, readCallback] = createAsyncQuery<std::vector<uint8_t>>([&, sharedConnection](auto&& res) {
                                                                       readFuncDisconneted(std::move(res), read_flag, actual_vec, sharedConnection, globalEnvironment);
                                                                   },
                                                                   [] {}, globalEnvironment, false, true);
    sharedConnection->read(readCallback);
}

/**
Prerequisites: the user must be allowed to run sudo ip without password
*/
TEST(HttpsConnection, ReadWhenNetworkAdapterDown) {

#ifndef SIRIUS_CONTRACT_RUN_SUDO_TESTS
    GTEST_SKIP();
#endif

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    std::vector<uint8_t> actual_vec;
    bool read_flag = false;
    std::string default_interface = exec_https("route | grep '^default' | grep -o '[^ ]*$'");
    ASSERT_NE(default_interface.length(), 0);
    std::string interface(default_interface.begin(), default_interface.end() - 1);

    threadManager.execute([&] {

        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://en.wikipedia.org/wiki/Byzantine_Empire");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_TRUE(connection);

                    auto sharedConnection = std::make_shared<InternetConnection>(std::move(*connection));

                    auto[_, readCallback] = createAsyncQuery<std::vector<uint8_t>>(
                            [&, sharedConnection](auto&& res) {
                                readFuncDisconneted(std::move(res), read_flag, actual_vec, sharedConnection,
                                                    globalEnvironment);
                            },
                            [] {}, globalEnvironment, false, true);

                    sharedConnection->read(readCallback);
                    std::ostringstream ss;
                    ss << "sudo ip link set " << interface << " down";
                    // std::cout << ss.str() << std::endl;
                    exec_https(ss.str().c_str());
                },
                [] {},
                globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
    ASSERT_TRUE(read_flag);
    std::ostringstream ss;
    ss << "sudo ip link set " << interface << " up";
    // std::cout << ss.str() << std::endl;
    exec_https(ss.str().c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(20000)); // Give the OS some time to reboot the interface
}

/**
Prerequisites: the user must be allowed to run sudo iptables and sudo ip6tables without password
*/
TEST(HttpsConnection, ConnectWhenBlockingConnection) {

#ifndef SIRIUS_CONTRACT_RUN_SUDO_TESTS
    GTEST_SKIP();
#endif

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    exec_https("sudo iptables -A INPUT -s 93.184.216.34 -j DROP");
    exec_https("sudo ip6tables -A INPUT -s 2606:2800:220:1:248:1893:25c8:1946 -j DROP");
    threadManager.execute([&] {
        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);
        auto urlDescription = parseURL("https://example.com");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_FALSE(connection);
                },
                [] {},
                globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
    exec_https("sudo iptables -D INPUT 1");
    exec_https("sudo ip6tables -D INPUT 1");
}

/**
Prerequisites: the user must be allowed to run sudo iptables and sudo ip6tables without password
*/
TEST(HttpsConnection, ReadWhenBlockingConnection) {

#ifndef SIRIUS_CONTRACT_RUN_SUDO_TESTS
    GTEST_SKIP();
#endif

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    std::vector<uint8_t> actual_vec;
    bool read_flag = false;

    threadManager.execute([&] {
        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://en.wikipedia.org/wiki/Byzantine_Empire");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_TRUE(connection);
                    auto sharedConnection = std::make_shared<InternetConnection>(std::move(*connection));
                    auto[_, readCallback] = createAsyncQuery<std::vector<uint8_t>>(
                            [&, sharedConnection](auto&& res) {
                                readFuncDisconneted(std::move(res), read_flag, actual_vec, sharedConnection,
                                                    globalEnvironment);
                            },
                            [] {}, globalEnvironment, false, true);

                    sharedConnection->read(readCallback);
                    exec_https("sudo iptables -A INPUT -s 103.102.166.224 -j DROP");
                    exec_https("sudo ip6tables -A INPUT -s 2001:df2:e500:ed1a::1 -j DROP");
                },
                [] {},
                globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
    exec_https("sudo iptables -D INPUT 1");
    exec_https("sudo ip6tables -D INPUT 1");
}

TEST(HttpsConnection, ValidCertificate) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    threadManager.execute([&] {

        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://example.com");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_TRUE(connection);
                }, [] {}, globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
}

TEST(HttpsConnection, TerminateCall) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    bool read_flag = false;
    bool terminate_flag = false;

    threadManager.execute([&] {

        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://example.com");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[query, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    read_flag = true;
                    ASSERT_TRUE(connection);
                }, [&]() {
                    terminate_flag = true;
                }, globalEnvironment, false, false);

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
                                                         connectionCallback);
        query->terminate();
    });
    threadManager.stop();
    ASSERT_FALSE(read_flag);
    ASSERT_TRUE(terminate_flag);
}

TEST(HttpsConnection, NonExisting) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();


    threadManager.execute([&] {
        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://examples123.com");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_FALSE(connection);
                },
                [] {}, globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
}

TEST(HttpsConnection, NonExistingTarget) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    bool read_flag = false;
    threadManager.execute([&] {

        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://google.com/eg");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_TRUE(connection);
                    auto sharedConnection = std::make_shared<InternetConnection>(std::move(*connection));
                    auto[_, readCallback] = createAsyncQuery<std::vector<uint8_t>>(
                            [&read_flag, sharedConnection](auto&& res) {
                                read_flag = true;
                                ASSERT_TRUE(res.has_value());
                                std::string actual(res->begin(), res->end());
                                const std::string expected = "<!DOCTYPE html>\n<html lang=en>\n  <meta charset=utf-8>\n  <meta name=viewport content=\"initial-scale=1, minimum-scale=1, width=device-width\">\n  <title>Error 404 (Not Found)!!1</title>\n  <style>\n    *{margin:0;padding:0}html,code{font:15px/22px arial,sans-serif}html{background:#fff;color:#222;padding:15px}body{margin:7% auto 0;max-width:390px;min-height:180px;padding:30px 0 15px}* > body{background:url(//www.google.com/images/errors/robot.png) 100% 5px no-repeat;padding-right:205px}p{margin:11px 0 22px;overflow:hidden}ins{color:#777;text-decoration:none}a img{border:0}@media screen and (max-width:772px){body{background:none;margin-top:0;max-width:none;padding-right:0}}#logo{background:url(//www.google.com/images/branding/googlelogo/1x/googlelogo_color_150x54dp.png) no-repeat;margin-left:-5px}@media only screen and (min-resolution:192dpi){#logo{background:url(//www.google.com/images/branding/googlelogo/2x/googlelogo_color_150x54dp.png) no-repeat 0% 0%/100% 100%;-moz-border-image:url(//www.google.com/images/branding/googlelogo/2x/googlelogo_color_150x54dp.png) 0}}@media only screen and (-webkit-min-device-pixel-ratio:2){#logo{background:url(//www.google.com/images/branding/googlelogo/2x/googlelogo_color_150x54dp.png) no-repeat;-webkit-background-size:100% 100%}}#logo{display:inline-block;height:54px;width:150px}\n  </style>\n  <a href=//www.google.com/><span id=logo aria-label=Google></span></a>\n  <p><b>404.</b> <ins>That\xE2\x80\x99s an error.</ins>\n  <p>The requested URL <code>/eg</code> was not found on this server.  <ins>That\xE2\x80\x99s all we know.</ins>\n";
                                ASSERT_EQ(actual, expected);
                            },
                            [] {}, globalEnvironment, false, false);
                    sharedConnection->read(readCallback);
                },
                [] {}, globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
    ASSERT_TRUE(read_flag);
}

TEST(HttpsConnection, RevokedCertificate) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    threadManager.execute([&] {

        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://revoked.badssl.com/");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_TRUE(!connection);
                }, [] {}, globalEnvironment, false, false);

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
                                                         connectionCallback);
    });

    threadManager.stop();
}

TEST(HttpsConnection, ExpiredCertificate) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    threadManager.execute([&] {
        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://expired.badssl.com/");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_TRUE(!connection);
                }, [] {}, globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
}

TEST(HttpsConnection, UntrustedRootCertificate) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    threadManager.execute([&] {
        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://untrusted-root.badssl.com/");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_TRUE(!connection);
                }, [] {}, globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
}

TEST(HttpsConnection, SelfSignedCertificate) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    threadManager.execute([&] {

        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://self-signed.badssl.com/");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_TRUE(!connection);
                }, [] {}, globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
}

TEST(HttpsConnection, WrongHostCertificate) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    threadManager.execute([&] {

        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://wrong.host.badssl.com/");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_TRUE(!connection);
                }, [] {}, globalEnvironment, false, false);

        InternetConnection::buildHttpsInternetConnection(ctx,
                                                         globalEnvironment,
                                                         urlDescription->host,
                                                         urlDescription->port,
                                                         urlDescription->target,
                                                         16 * 1024,
                                                         30000,
                                                         500,
                                                         60,
                                                         RevocationVerificationMode::SOFT,
                                                         connectionCallback);
    });
    threadManager.stop();
}

TEST(HttpsConnection, PinningTest) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    threadManager.execute([&] {

        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://pinning-test.badssl.com/");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_TRUE(!connection);
                }, [] {}, globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
}

TEST(HttpsConnection, ClientCertMissing) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    threadManager.execute([&] {

        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://client-cert-missing.badssl.com/");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_TRUE(!connection);
                }, [] {}, globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
}

TEST(HttpsConnection, WeakSignature) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    threadManager.execute([&] {

        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("https://sha1-2017.badssl.com/");

        ASSERT_TRUE(urlDescription);
        ASSERT_TRUE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "443");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_TRUE(!connection);
                }, [] {}, globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
}

TEST(HttpsConnection, ConnectingNonHttpsURL) {

    GlobalEnvironmentImpl globalEnvironment;
    auto& threadManager = globalEnvironment.threadManager();

    threadManager.execute([&] {
        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);

        auto urlDescription = parseURL("http://example.com");

        ASSERT_TRUE(urlDescription);
        ASSERT_FALSE(urlDescription->ssl);
        ASSERT_EQ(urlDescription->port, "80");

        auto[_, connectionCallback] = createAsyncQuery<InternetConnection>(
                [&](auto&& connection) {
                    ASSERT_FALSE(connection);
                },
                [] {}, globalEnvironment, false, false);

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
                                                         connectionCallback);
    });
    threadManager.stop();
}

}