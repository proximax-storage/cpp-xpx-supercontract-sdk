/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "InternetConnection.h"

#include "boost/beast.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include "OCSPHandler.h"
#include "OCSPVerifier.h"

namespace sirius::contract {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class HttpInternetConnection:
        public InternetConnection,
        public std::enable_shared_from_this<HttpInternetConnection> {

private:


    enum class ConnectionState {
        UNINITIALIZED, INITIALIZED, CLOSED
    };


    ThreadManager& m_threadManager;

    std::string m_host;
    std::string m_port;
    std::string m_target;

    tcp::resolver m_resolver;
    beast::tcp_stream m_stream;
    beast::flat_buffer m_buffer;
    http::request<http::empty_body> m_req;
    http::parser<false, http::buffer_body> m_res;
    std::vector<uint8_t> m_readDataBuffer;

    int m_timeout;
    std::optional<boost::asio::high_resolution_timer> m_timeoutTimer;

    ConnectionState m_state = ConnectionState::UNINITIALIZED;

    const DebugInfo m_dbgInfo;

public:

    HttpInternetConnection(
            ThreadManager& threadManager,
            const std::string& host,
            const std::string& port,
            const std::string& target,
            int bufferSize,
            int timeout,
            const DebugInfo& debugInfo )
    : m_threadManager( threadManager )
    , m_host( host )
    , m_target( target )
    , m_resolver( threadManager.context() )
    , m_stream( threadManager.context() )
    , m_buffer( bufferSize )
    , m_timeout( timeout )
    , m_dbgInfo( debugInfo )
    {
        m_stream.expires_never();
        m_res.body_limit(-1);
    }

public:

    void open( std::weak_ptr<AbstractAsyncQuery<bool>> callback ) override {

        DBG_MAIN_THREAD

        _ASSERT( m_state == ConnectionState::UNINITIALIZED )

        m_req.version( 10 );
        m_req.method( http::verb::get );
        m_req.target( m_target );
        m_req.set( http::field::host, m_host );
        m_req.set( http::field::user_agent, BOOST_BEAST_VERSION_STRING );
        m_res.eager(true);

        // Look up the domain name
        m_resolver.async_resolve(
                m_host,
                m_port,
                beast::bind_front_handler(
                        [pThisWeak = weak_from_this(), callback] ( beast::error_code ec,
                                                        tcp::resolver::results_type results ) {
                            if ( auto pThis = pThisWeak.lock(); pThis ) {
                                pThis->onHostResolved( ec, results, callback );
                            }
                        } )
        );

        runTimeoutTimer();
    }

    void read( std::weak_ptr<AbstractAsyncQuery<std::optional<std::vector<uint8_t>>>> callback ) override {

        DBG_MAIN_THREAD

        auto c = callback.lock();

        if ( !c ) {
            close();
            return;
        }

        if ( m_res.is_done()) {
            close();
            c->postReply( std::vector<uint8_t>());
            return;
        }

        m_res.get().body().data = m_readDataBuffer.data();
        m_res.get().body().size = m_readDataBuffer.size();

        http::async_read_some(
                m_stream,
                m_buffer,
                m_res,
                beast::bind_front_handler( [pThisWeak = weak_from_this(), callback](
                        beast::error_code ec,
                        std::size_t bytes_transferred ) {
                    if ( auto pThis = pThisWeak.lock(); pThis ) {
                        pThis->onRead( ec, bytes_transferred, callback );
                    }
                } ));

        runTimeoutTimer();
    }

    void close() override {

        DBG_MAIN_THREAD

        if ( m_state == ConnectionState::CLOSED ) {
            return;
        }

        m_state = ConnectionState::CLOSED;

        m_resolver.cancel();
        m_stream.close();
        m_timeoutTimer.reset();
    }

    ~HttpInternetConnection() override {

        DBG_MAIN_THREAD

        _ASSERT( m_state == ConnectionState::CLOSED )
    }

private:

    void
    onHostResolved(
            beast::error_code ec,
            const tcp::resolver::results_type& results,
            std::weak_ptr<AbstractAsyncQuery<bool>> callback ) {

        DBG_MAIN_THREAD

        m_timeoutTimer.reset();

        auto c = callback.lock();

        if ( !c ) {
            close();
            return;
        }

        if ( ec ) {
            close();
            c->postReply( false );
            return;
        }

        // Make the connection on the IP address we get from a lookup
        m_stream.async_connect(
                results,
                beast::bind_front_handler(
                        [pThisWeak = weak_from_this(), callback](
                                beast::error_code ec,
                                tcp::resolver::results_type::endpoint_type results) {
                            if ( auto pThis = pThisWeak.lock(); pThis ) {
                                pThis->onConnected( ec, results, callback );
                            }
                        }
                ));

        runTimeoutTimer();
    }

    void
    onConnected( beast::error_code ec,
                 tcp::resolver::results_type::endpoint_type,
                 std::weak_ptr<AbstractAsyncQuery<bool>> callback ) {

        DBG_MAIN_THREAD

        m_timeoutTimer.reset();

        auto c = callback.lock();

        if ( !c ) {
            close();
            return;
        }

        if ( ec ) {
            close();
            c->postReply( false );
            return;
        }

        http::async_write( m_stream, m_req,
                           beast::bind_front_handler( [pThisWeak = weak_from_this(), callback]( beast::error_code ec,
                                                                                                std::size_t bytes_transferred ) {
                               if ( auto pThis = pThisWeak.lock(); pThis ) {
                                   pThis->onWritten( ec, bytes_transferred, callback );
                               }
                           } ));

        runTimeoutTimer();
    }

    void onWritten( beast::error_code ec,
                    std::size_t bytes_transferred,
                    std::weak_ptr<AbstractAsyncQuery<bool>> callback ) {

        DBG_MAIN_THREAD

        m_timeoutTimer.reset();

        auto c = callback.lock();

        if ( !c ) {
            close();
            return;
        }

        if ( ec ) {
            close();
            c->postReply( false );
            return;
        }

        m_state = ConnectionState::INITIALIZED;
        c->postReply( true );
    }

    void onRead(
            beast::error_code ec,
            std::size_t bytes_transferred,
            std::weak_ptr<AbstractAsyncQuery<std::optional<std::vector<uint8_t>>>> callback ) {

        DBG_MAIN_THREAD

        _ASSERT( m_state != ConnectionState::UNINITIALIZED )

        m_timeoutTimer.reset();

        auto c = callback.lock();

        if ( !c ) {
            close();
            return;
        }

        if ( ec == http::error::need_buffer ) {
            ec.assign(0, ec.category());
        }

        if( ec ) {
            close();
            c->postReply( {} );
            return;
        }

        if ( m_res.is_done() || m_res.get().body().size == 0 ) {
            auto readSize = m_readDataBuffer.size() - m_res.get().body().size;
            std::vector<uint8_t> data( readSize );
            std::copy( m_readDataBuffer.begin(), m_readDataBuffer.begin() + readSize, data.begin() );
            c->postReply( std::move(data) );
        } else {
            http::async_read_some(
                    m_stream,
                    m_buffer,
                    m_res,
                    beast::bind_front_handler( [pThisWeak = weak_from_this(), callback](
                            beast::error_code ec,
                            std::size_t bytes_transferred ) {
                        if ( auto pThis = pThisWeak.lock(); pThis ) {
                            pThis->onRead( ec, bytes_transferred, callback );
                        }
                    } ));
            runTimeoutTimer();
        }
    }

    void runTimeoutTimer() {

        DBG_MAIN_THREAD

        _ASSERT( !m_timeoutTimer )

        m_timeoutTimer = m_threadManager.startTimer( m_timeout, [pThisWeak=weak_from_this()] {
            if ( auto pThis = pThisWeak.lock(); pThis ) {
                pThis->close();
            }
        } );
    }
};

}