/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <memory>
#include "boost/beast.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/ssl.hpp>

#include "supercontract/GlobalEnvironment.h"
#include "supercontract/SingleThread.h"
#include "internet/InternetResource.h"
#include "OCSPVerifier.h"

namespace sirius::contract::internet {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

enum class RevocationVerificationMode {
    SOFT, HARD
};

class HttpsInternetResource:
        private SingleThread,
        public InternetResource,
        public std::enable_shared_from_this<HttpsInternetResource> {

private:

    enum class ConnectionState {
        UNINITIALIZED, INITIALIZED, SHUTDOWN, CLOSED
    };

    GlobalEnvironment& m_environment;

    std::string m_host;
    std::string m_port;
    std::string m_target;

    tcp::resolver                           m_resolver;
    beast::ssl_stream<beast::tcp_stream>    m_stream;
    beast::flat_buffer                      m_buffer;
    http::request<http::empty_body>         m_req;
    http::parser<false, http::buffer_body>  m_res;
    std::vector<uint8_t>                    m_readDataBuffer;

    std::map<RequestId, std::unique_ptr<OCSPVerifier>> m_ocspVerifiers;
    bool                                               m_handshakeReceived = false;

    int m_connectionTimeout;
    int m_ocspQueryTimerDelay;
    int m_ocspQueryMaxEfforts;
    Timer m_timeoutTimer;

    RevocationVerificationMode m_revocationVerificationMode;

    ConnectionState m_state = ConnectionState::UNINITIALIZED;

public:

    HttpsInternetResource(
            ssl::context& ctx,
            GlobalEnvironment& globalEnvironment,
            const std::string& host,
            const std::string& port,
            const std::string& target,
            int bufferSize,
            int connectionTimeout,
            int ocspQueryTimerDelay,
            int ocspQueryMaxEfforts,
            RevocationVerificationMode mode );

public:

    void open( const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback ) override;

    void read( const std::shared_ptr<AsyncQueryCallback<std::optional<std::vector<uint8_t>>>>& callback ) override;

    void close() override;

    ~HttpsInternetResource() override;

private:

    void onHostResolved(
            beast::error_code ec,
            const tcp::resolver::results_type& results,
            const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback );

    void
    onConnected( beast::error_code ec,
                 const tcp::resolver::results_type::endpoint_type&,
                 const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback );

    void onHandshake( beast::error_code ec, const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback );

    void onWritten( beast::error_code ec,
                    std::size_t bytes_transferred,
                    const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback );

    void onRead(
            beast::error_code ec,
            std::size_t bytes_transferred,
            const std::shared_ptr<AsyncQueryCallback<std::optional<std::vector<uint8_t>>>>& callback );

    void runTimeoutTimer();

    void verifyOCSP( ssl::verify_context& c, const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback );

    void onOCSPVerified( const RequestId& requestId, CertificateRevocationCheckStatus status, const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback );

    void onExtendedHandshake( const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback );

    void onShutdown( beast::error_code ec );
};

}