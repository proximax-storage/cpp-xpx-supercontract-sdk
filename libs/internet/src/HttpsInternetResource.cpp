/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "HttpsInternetResource.h"

#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include "OCSPHandler.h"
#include "OCSPVerifier.h"
#include "utils/Random.h"
#include "supercontract/Identifiers.h"
#include "supercontract/AsyncQuery.h"
#include <openssl/sslerr.h>

namespace sirius::contract::internet {

HttpsInternetResource::HttpsInternetResource(ssl::context& ctx, GlobalEnvironment& globalEnvironment,
                                             const std::string& host, const std::string& port,
                                             const std::string& target, int bufferSize, int connectionTimeout,
                                             int ocspQueryTimerDelay, int ocspQueryMaxEfforts,
                                             RevocationVerificationMode mode)
        : m_environment(globalEnvironment)
        , m_host(host)
        , m_port(port)
        , m_target(target)
        , m_resolver(globalEnvironment.threadManager().context())
        , m_stream(globalEnvironment.threadManager().context(), ctx)
        , m_buffer(bufferSize)
        , m_readDataBuffer(bufferSize)
        , m_connectionTimeout(connectionTimeout)
        , m_ocspQueryTimerDelay(ocspQueryTimerDelay)
        , m_ocspQueryMaxEfforts(ocspQueryMaxEfforts)
        , m_revocationVerificationMode(mode) {
    beast::get_lowest_layer(m_stream).expires_never();
    m_res.body_limit(-1);
}

HttpsInternetResource::~HttpsInternetResource() {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_state == ConnectionState::CLOSED, m_environment.logger())
}

void HttpsInternetResource::open(std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_state == ConnectionState::UNINITIALIZED, m_environment.logger())

    if (callback->isTerminated()) {
        m_environment.logger().warn("Open Https Connection Empty Callback");
        closeDuringInitialization();
        return;
    }

    ASSERT (SSL_set_tlsext_host_name(m_stream.native_handle(), m_host.c_str()), m_environment.logger());

    ASSERT(SSL_set1_host(m_stream.native_handle(), m_host.c_str()), m_environment.logger());

    m_req.version(10);
    m_req.method(http::verb::get);
    m_req.target(m_target);
    m_req.set(http::field::host, m_host);
    m_req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    m_res.eager(true);

    // Look up the domain name
    m_resolver.async_resolve(
            m_host,
            m_port,
            beast::bind_front_handler(
                    [pThis = shared_from_this(), callback](beast::error_code ec,
                                                           const tcp::resolver::results_type& results) {
                        pThis->onHostResolved(ec, results, callback);
                    })
    );

    m_timeoutTimer = m_environment.threadManager().startTimer(m_connectionTimeout, [pThisWeak = weak_from_this(), callback] {
        if (auto pThis = pThisWeak.lock(); pThis) {
            pThis->onHostResolved(boost::asio::error::operation_aborted, {}, callback);
        }
    });
}

void HttpsInternetResource::read(
        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_state >= ConnectionState::INITIALIZED, m_environment.logger())

    if (callback->isTerminated()) {
        m_environment.logger().warn("Read Https Connection Empty Callback");
        return;
    }

    if (m_state == ConnectionState::SHUTDOWN || m_state == ConnectionState::CLOSED) {
        m_environment.logger().warn("Read Https Connection Closed");
        callback->postReply(tl::unexpected(std::make_error_code(std::errc::bad_file_descriptor)));
        return;
    }

    if (m_res.is_done()) {
        m_environment.logger().info("Read Https Connection Finished");
        callback->postReply(std::vector<uint8_t>());
        return;
    }

    m_res.get().body().data = m_readDataBuffer.data();
    m_res.get().body().size = m_readDataBuffer.size();

    http::async_read_some(
            m_stream,
            m_buffer,
            m_res,
            beast::bind_front_handler([pThis = shared_from_this(), callback](
                    beast::error_code ec,
                    std::size_t bytes_transferred) {
                pThis->onRead(ec, bytes_transferred, callback);
            }));

    runTimeoutTimer();
}

void HttpsInternetResource::close() {

    ASSERT(isSingleThread(), m_environment.logger())

    if (m_state == ConnectionState::SHUTDOWN || m_state == ConnectionState::CLOSED) {
        return;
    }

    m_state = ConnectionState::SHUTDOWN;

    m_ocspVerifiers.clear();

    beast::get_lowest_layer(m_stream).cancel();

    m_stream.async_shutdown([pThis = shared_from_this()](beast::error_code ec) {
        pThis->onShutdown(ec);
    });

    m_timeoutTimer = m_environment.threadManager().startTimer(m_connectionTimeout, [pThisWeak = weak_from_this()] {
        if (auto pThis = pThisWeak.lock(); pThis) {
            // It should cause an error on shutdown
            beast::get_lowest_layer(pThis->m_stream).cancel();
        }
    });
}

void HttpsInternetResource::onHostResolved(beast::error_code ec,
                                           const boost::asio::ip::basic_resolver<tcp, boost::asio::executor>::results_type& results,
                                           const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    m_timeoutTimer.cancel();

    if (m_state != ConnectionState::UNINITIALIZED) {
        // The resolve has already been canceled by the timer
        return;
    }

    if (callback->isTerminated()) {
        m_environment.logger().warn("OnHostResolved Https Connection Empty Callback");
        closeDuringInitialization();
        return;
    }

    if (ec) {
        m_environment.logger().warn("OnHostResolved Https Connection Error: {}", ec.message());
        closeDuringInitialization();
        callback->postReply(tl::unexpected(ec));
        return;
    }

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(m_stream).async_connect(
            results,
            beast::bind_front_handler(
                    [pThis = shared_from_this(), callback](
                            beast::error_code ec,
                            const tcp::resolver::results_type::endpoint_type& results) {
                        pThis->onConnected(ec, results, callback);
                    }
            ));

    runTimeoutTimer();
}

void HttpsInternetResource::onConnected(beast::error_code ec, const boost::asio::ip::basic_endpoint<tcp>&,
                                        const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    m_timeoutTimer.cancel();

    if (callback->isTerminated()) {
        m_environment.logger().warn("OnConnected Https Connection Empty Callback");
        closeDuringInitialization();
        return;
    }

    if (ec) {
        m_environment.logger().warn("OnConnected Https Connection Error: {}", ec.message());
        closeDuringInitialization();
        callback->postReply(tl::unexpected(ec));
        return;
    }

    m_stream.set_verify_callback([=, pThis = shared_from_this()](bool p, ssl::verify_context& c) -> bool {
        if (pThis->m_state == ConnectionState::SHUTDOWN) {
            // Is It Possible?
            return false;
        }

        if (!p) {
            return false;
        }

        pThis->verifyOCSP(c, callback);

        return true;
    });

    m_stream.async_handshake(
            ssl::stream_base::client,
            beast::bind_front_handler([pThis = shared_from_this(), callback](beast::error_code ec) {
                pThis->onHandshake(ec, callback);
            }));

    runTimeoutTimer();
}

void HttpsInternetResource::onHandshake(beast::error_code ec,
                                        const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(!m_handshakeReceived, m_environment.logger())

    m_timeoutTimer.cancel();

    if (callback->isTerminated()) {
        m_environment.logger().warn("OnHandshake Https Connection Empty Callback");
        closeDuringInitialization();
        return;
    }

    if (ec) {
        m_environment.logger().warn("OnHandshake Https Connection Error {}",  ec.message());
        closeDuringInitialization();
        callback->postReply(tl::unexpected(ec));
        return;
    }

    m_handshakeReceived = true;

    if (m_ocspVerifiers.empty()) {
        onExtendedHandshake(callback);
    }
}

void HttpsInternetResource::onWritten(beast::error_code ec, std::size_t bytes_transferred,
                                      const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    m_timeoutTimer.cancel();

    if (callback->isTerminated()) {
        m_environment.logger().warn("OnWritten Https Connection Empty Callback");
        close();
        return;
    }

    if (ec) {
        m_environment.logger().warn("OnWritten Https Connection Error: {}", ec.message());
        close();
        callback->postReply(tl::unexpected(ec));
        return;
    }

    m_state = ConnectionState::INITIALIZED;
    m_environment.logger().info("Established Connection With {}:{}", m_host, m_port);
    callback->postReply(shared_from_this());
}

void HttpsInternetResource::onRead(beast::error_code ec, std::size_t bytes_transferred,
                                   const std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>>& callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    m_timeoutTimer.cancel();

    if (callback->isTerminated()) {
        m_environment.logger().warn("OnRead Https Connection Empty Callback");
        return;
    }

    if (ec == http::error::need_buffer) {
        ec.assign(0, ec.category());
    }

    if (ec) {
        m_environment.logger().warn("OnRead Https Connection Error {}", ec.message());
        callback->postReply(tl::unexpected(ec));
        return;
    }

    if (m_res.is_done() || m_res.get().body().size == 0) {
        auto readSize = m_readDataBuffer
                                .size() - m_res.get().body().size;
        std::vector<uint8_t> data(readSize);
        std::copy(m_readDataBuffer.begin(), m_readDataBuffer.begin() + readSize, data.begin());
        callback->postReply(std::move(data));
    } else {
        http::async_read_some(
                m_stream,
                m_buffer,
                m_res,
                beast::bind_front_handler([pThis = shared_from_this(), callback](
                        beast::error_code ec,
                        std::size_t bytes_transferred) {
                    pThis->onRead(ec, bytes_transferred, callback);
                }));
        runTimeoutTimer();
    }
}

void HttpsInternetResource::runTimeoutTimer() {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(!m_timeoutTimer, m_environment.logger())

    m_timeoutTimer = m_environment.threadManager().startTimer(m_connectionTimeout, [pThisWeak = weak_from_this()] {
        if (auto pThis = pThisWeak.lock(); pThis) {
            beast::get_lowest_layer(pThis->m_stream).cancel();
        }
    });
}

void HttpsInternetResource::verifyOCSP(ssl::verify_context& c,
                                       const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    auto* handle = c.native_handle();

    auto* cert = X509_STORE_CTX_get_current_cert(handle);
    auto* issuer = X509_STORE_CTX_get0_current_issuer(handle);

    if (cert == issuer) {
        return;
    }
    auto requestId = utils::generateRandomByteValue<RequestId>();

    auto ocspVerifier = std::make_unique<OCSPVerifier>(
            X509_STORE_CTX_get0_store(handle),
            X509_STORE_CTX_get1_chain(handle),
            [=, pThis = shared_from_this()](CertificateRevocationCheckStatus status) {
                pThis->onOCSPVerified(requestId, status, callback);
            },
            m_environment,
            m_ocspQueryTimerDelay,
            m_ocspQueryMaxEfforts);
    auto[it, success] = m_ocspVerifiers.emplace(requestId, std::move(ocspVerifier));
    it->second->run(cert, issuer);
}

void HttpsInternetResource::onOCSPVerified(const RequestId& requestId, CertificateRevocationCheckStatus status,
                                           const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    if (callback->isTerminated()) {
        m_environment.logger().warn("OnOCSPVerified Https Connection Empty Callback");
        return;
    }

    auto it = m_ocspVerifiers.find(requestId);

    if (it == m_ocspVerifiers.end()) {
        return;
    }

    m_ocspVerifiers.erase(it);

    bool ocspValid = true;

    if (m_revocationVerificationMode == RevocationVerificationMode::SOFT &&
        status == CertificateRevocationCheckStatus::REVOKED
        or
        m_revocationVerificationMode == RevocationVerificationMode::HARD &&
        status != CertificateRevocationCheckStatus::VALID) {
        ocspValid = false;
    }


    if (!ocspValid) {
        m_environment.logger().warn("OnOCSPVerified Https Connection OCSP Invalid");
        closeDuringInitialization();
        auto ec = beast::error_code(SSL_R_CERTIFICATE_VERIFY_FAILED, boost::asio::error::get_ssl_category());
        callback->postReply(tl::unexpected(ec));
        return;
    }

    if (m_ocspVerifiers.empty() && m_handshakeReceived) {
        onExtendedHandshake(callback);
    }
}

void HttpsInternetResource::onExtendedHandshake(
        const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback) {
    http::async_write(m_stream, m_req,
                      beast::bind_front_handler([pThis = shared_from_this(), callback](beast::error_code ec,
                                                                                       std::size_t bytes_transferred) {
                          pThis->onWritten(ec, bytes_transferred, callback);
                      }));

    runTimeoutTimer();
}

void HttpsInternetResource::closeDuringInitialization() {

    ASSERT(isSingleThread(), m_environment.logger())

    if (m_state == ConnectionState::SHUTDOWN || m_state == ConnectionState::CLOSED) {
        return;
    }

    m_ocspVerifiers.clear();

    m_state = ConnectionState::SHUTDOWN;

    onShutdown({});
}

void HttpsInternetResource::onShutdown(beast::error_code ec) {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_state == ConnectionState::SHUTDOWN, m_environment.logger());

    if (ec == net::error::eof) {

    }

    if (ec) {
        // Error Is Normal Here
        m_environment.logger().warn("Error During Shutdown: {}", ec.message());
    }

    m_state = ConnectionState::CLOSED;

    m_timeoutTimer.cancel();

    beast::get_lowest_layer(m_stream).close();
}

}