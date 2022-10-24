/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "HttpInternetResource.h"

namespace sirius::contract::internet {

HttpInternetResource::HttpInternetResource(GlobalEnvironment& globalEnvironment,
                                           const std::string& host,
                                           const std::string& port,
                                           const std::string& target,
                                           int bufferSize,
                                           int timeout)
        : m_environment(globalEnvironment)
        , m_port(port)
        , m_host(host)
        , m_target(target)
        , m_resolver(globalEnvironment.threadManager().context())
        , m_stream(globalEnvironment.threadManager().context())
        , m_buffer(bufferSize)
        , m_readDataBuffer(bufferSize)
        , m_timeout(timeout) {
    m_stream.expires_never();
    m_res.body_limit(-1);
}

HttpInternetResource::~HttpInternetResource() {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_state == ConnectionState::CLOSED, m_environment.logger())
}

void HttpInternetResource::open(std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_state == ConnectionState::UNINITIALIZED, m_environment.logger())

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
                                                           tcp::resolver::results_type results) {
                        pThis->onHostResolved(ec, results, callback);
                    })
    );

    m_timeoutTimer = m_environment.threadManager().startTimer(m_timeout, [pThisWeak = weak_from_this(), callback] {
        if (auto pThis = pThisWeak.lock(); pThis) {
            pThis->onHostResolved(boost::asio::error::operation_aborted, {}, callback);
        }
    });
}

void HttpInternetResource::read(std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {

    ASSERT(isSingleThread(), m_environment.logger())

    if (callback->isTerminated()) {
        return;
    }

    if (m_res.is_done()) {
        callback->postReply(std::vector<uint8_t>());
        return;
    }

    m_res.get().body().data = m_readDataBuffer.data();
    m_res.get().body().size = m_readDataBuffer.size();

    http::async_read_some(
            m_stream,
            m_buffer,
            m_res,
            beast::bind_front_handler([pThisWeak = weak_from_this(), callback](
                    beast::error_code ec,
                    std::size_t bytes_transferred) {
                if (auto pThis = pThisWeak.lock(); pThis) {
                    pThis->onRead(ec, bytes_transferred, callback);
                }
            }));

    runTimeoutTimer();
}

void HttpInternetResource::close() {

    ASSERT(isSingleThread(), m_environment.logger())

    if (m_state == ConnectionState::CLOSED) {
        return;
    }

    m_state = ConnectionState::CLOSED;

    m_stream.close();
    m_timeoutTimer.cancel();
}

void HttpInternetResource::onHostResolved(beast::error_code ec,
                                          const boost::asio::ip::basic_resolver<tcp, boost::asio::executor>::results_type& results,
                                           const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback ) {

    ASSERT(isSingleThread(), m_environment.logger())

    m_timeoutTimer.cancel();

    if (m_state == ConnectionState::CLOSED) {
        // The resolve has already been canceled by the timer
        return;
    }

    if (callback->isTerminated()) {
        m_environment.logger().warn("OnHostResolved Http Connection Empty Callback");
        close();
        return;
    }

    if (ec) {
        m_environment.logger().warn("OnHostResolved Http Connection Error: {}", ec.message());
        close();
        callback->postReply(tl::unexpected(ec));
        return;
    }

    // Make the connection on the IP address we get from a lookup
    m_stream.async_connect(
            results,
            beast::bind_front_handler(
                    [pThis = shared_from_this(), callback](
                            beast::error_code ec,
                            tcp::resolver::results_type::endpoint_type results) {
                        pThis->onConnected(ec, results, callback);
                    }
            ));

    runTimeoutTimer();
}

void HttpInternetResource::onConnected(beast::error_code ec, tcp::resolver::results_type::endpoint_type,
                                        const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback ) {

    ASSERT(isSingleThread(), m_environment.logger())

    m_timeoutTimer.cancel();

    if (callback->isTerminated()) {
        m_environment.logger().warn("Open Http Connection Empty Callback");
        close();
        return;
    }

    if (ec) {
        m_environment.logger().warn("OnConnected Http Connection Error: {}", ec.message());
        close();
        callback->postReply(tl::unexpected(ec));
        return;
    }

    http::async_write(m_stream, m_req,
                      beast::bind_front_handler([pThis = shared_from_this(), callback](beast::error_code ec,
                                                                                       std::size_t bytes_transferred) {
                          pThis->onWritten(ec, bytes_transferred, callback);
                      }));

    runTimeoutTimer();
}

void HttpInternetResource::onWritten(beast::error_code ec, std::size_t bytes_transferred,
                                      const std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>& callback ) {

    ASSERT(isSingleThread(), m_environment.logger())

    m_timeoutTimer.cancel();

    if (callback->isTerminated()) {
        m_environment.logger().warn("OnWritten Http Connection Empty Callback");
        close();
        return;
    }

    if (ec) {
        m_environment.logger().warn("OnWritten Http Connection Error: {}", ec.message());
        close();
        callback->postReply(tl::unexpected(ec));
        return;
    }

    m_state = ConnectionState::INITIALIZED;
    callback->postReply(shared_from_this());
}

void HttpInternetResource::onRead(beast::error_code ec, std::size_t bytes_transferred,
                                   const std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>>& callback ) {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_state >= ConnectionState::INITIALIZED, m_environment.logger())

    m_timeoutTimer.cancel();

    if (callback->isTerminated()) {
        m_environment.logger().warn("OnRead Http Connection Empty Callback");
        return;
    }

    if (ec == http::error::need_buffer) {
        ec.assign(0, ec.category());
    }

    if (ec) {
        m_environment.logger().warn("OnRead Http Connection Error {}", ec.message());
        callback->postReply(tl::unexpected(ec));        return;
    }

    if (m_res.is_done() || m_res.get().body().size == 0) {
        auto readSize = m_readDataBuffer.size() - m_res.get().body().size;
        std::vector<uint8_t> data(readSize);
        std::copy(m_readDataBuffer.begin(), m_readDataBuffer.begin() + readSize, data.begin());
        callback->postReply(std::move(data));
    } else {
        http::async_read_some(
                m_stream,
                m_buffer,
                m_res,
                beast::bind_front_handler([pThisWeak = weak_from_this(), callback](
                        beast::error_code ec,
                        std::size_t bytes_transferred) {
                    if (auto pThis = pThisWeak.lock(); pThis) {
                        pThis->onRead(ec, bytes_transferred, callback);
                    }
                }));
        runTimeoutTimer();
    }
}

void HttpInternetResource::runTimeoutTimer() {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(!m_timeoutTimer, m_environment.logger())

    m_timeoutTimer = m_environment.threadManager().startTimer(m_timeout, [pThisWeak = weak_from_this()] {
        if (auto pThis = pThisWeak.lock(); pThis) {
            pThis->close();
        }
    });
}

}