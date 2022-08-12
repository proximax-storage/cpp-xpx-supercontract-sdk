/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once


#include "boost/beast.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/ssl.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include "internet/OCSPHandler.h"
#include "internet/OCSPVerifier.h"
#include "utils/Random.h"
#include "internet/InternetConnection.h"
#include "supercontract/ThreadManager.h"
#include "supercontract/Identifiers.h"
#include "../../contract/DebugInfo.h"
#include "supercontract/AsyncQuery.h"
#include "../../contract/log.h"

namespace sirius::contract::internet {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

enum class RevocationVerificationMode {
    SOFT, HARD
};

class HttpsInternetConnection:
        public InternetConnection,
        public std::enable_shared_from_this<HttpsInternetConnection> {

private:

    enum class ConnectionState {
        UNINITIALIZED, INITIALIZED, SHUTDOWN, CLOSED
    };

    ThreadManager& m_threadManager;

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
    std::optional<boost::asio::high_resolution_timer> m_timeoutTimer;

    RevocationVerificationMode m_revocationVerificationMode;

    ConnectionState m_state = ConnectionState::UNINITIALIZED;

    const DebugInfo m_dbgInfo;

public:

    HttpsInternetConnection(
            ssl::context& ctx,
            ThreadManager& threadManager,
            const std::string& host,
            const std::string& port,
            const std::string& target,
            int bufferSize,
            int connectionTimeout,
            int ocspQueryTimerDelay,
            int ocspQueryMaxEfforts,
            RevocationVerificationMode mode,
            const DebugInfo& debugInfo )
    : m_threadManager( threadManager )
    , m_host( host )
    , m_port( port )
    , m_target( target )
    , m_resolver( threadManager.context() )
    , m_stream( threadManager.context(), ctx )
    , m_readDataBuffer( bufferSize )
    , m_connectionTimeout( connectionTimeout )
    , m_ocspQueryTimerDelay( ocspQueryTimerDelay )
    , m_ocspQueryMaxEfforts( ocspQueryMaxEfforts )
    , m_revocationVerificationMode( mode )
    , m_dbgInfo( debugInfo )
    {
        beast::get_lowest_layer( m_stream ).expires_never();
        m_res.body_limit(-1);
    }

public:

    void open( std::weak_ptr<AbstractAsyncQuery<bool>> callback ) override {

        DBG_MAIN_THREAD

        _ASSERT( m_state == ConnectionState::UNINITIALIZED )

        auto c = callback.lock();

        if ( !c ) {
            _LOG_WARN( "Open Https Connection Empty Callback" );
            close();
            return;
        }

        if ( !SSL_set_tlsext_host_name( m_stream.native_handle(), m_host.c_str() )) {
            _LOG_WARN( "Open Https Connection SSL Set Host Name Error" );
            close();
            c->postReply( false );
            return;
        }

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

        _ASSERT( m_state != ConnectionState::UNINITIALIZED )

        auto c = callback.lock();

        if ( !c ) {
            _LOG_WARN( "Read Https Connection Empty Callback" );
            close();
            return;
        }

        if ( m_state == ConnectionState::SHUTDOWN || m_state == ConnectionState::CLOSED ) {
            _LOG_WARN( "Read Https Connection Closed" );
            c->postReply( {} );
            return;
        }

        if ( m_res.is_done() ) {
            _LOG_WARN( "Read Https Connection Finished" );
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

        if ( m_state == ConnectionState::SHUTDOWN || m_state == ConnectionState::CLOSED ) {
            return;
        }

        m_state = ConnectionState::SHUTDOWN;

        m_ocspVerifiers.clear();

        m_timeoutTimer.reset();
        m_resolver.cancel();
        beast::get_lowest_layer( m_stream ).cancel();

        m_stream.async_shutdown( [pThis = shared_from_this()] ( beast::error_code ec ) {
            pThis->onShutdown( ec );
        } );

        m_timeoutTimer = m_threadManager.startTimer( m_connectionTimeout, [pThisWeak=weak_from_this()]  {
            if ( auto pThis = pThisWeak.lock(); pThis ) {
                // It should cause an error on shutdown
                beast::get_lowest_layer( pThis->m_stream ).cancel();
            }
        });
    }

    ~HttpsInternetConnection() override {

        DBG_MAIN_THREAD

        _ASSERT( m_state == ConnectionState::CLOSED )
    }

private:

    void onHostResolved(
            beast::error_code ec,
            const tcp::resolver::results_type& results,
            std::weak_ptr<AbstractAsyncQuery<bool>> callback ) {

        DBG_MAIN_THREAD

        m_timeoutTimer.reset();

        auto c = callback.lock();

        if ( !c ) {
            _LOG_WARN( "OnHostResolved Https Connection Empty Callback" );
            close();
            return;
        }

        if ( ec ) {
            _LOG_WARN( "OnHostResolved Https Connection Error "  << ec.message() );
            close();
            c->postReply( false );
            return;
        }

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer( m_stream ).async_connect(
                results,
                beast::bind_front_handler(
                        [pThisWeak = weak_from_this(), callback](
                                beast::error_code ec,
                                tcp::resolver::results_type::endpoint_type results ) {
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
            _LOG_WARN( "OnConnected Https Connection Empty Callback" );
            close();
            return;
        }

        if ( ec ) {
            _LOG_WARN( "OnConnected Https Connection Error " << ec.message() );
            close();
            c->postReply( false );
            return;
        }

        m_stream.set_verify_callback( [=, pThisWeak = weak_from_this()]( bool p, ssl::verify_context& c ) -> bool {

            auto pThis = pThisWeak.lock();

            if ( !pThis ) {
                return false;
            }

            if ( pThis->m_state == ConnectionState::SHUTDOWN ) {
                // Is It Possible?
                return false;
            }

            if ( !p ) {
                return false;
            }

            pThis->verifyOCSP( c, callback );

            return true;
        } );

        m_stream.async_handshake(
                ssl::stream_base::client,
                beast::bind_front_handler( [pThisWeak = weak_from_this(), callback]( beast::error_code ec ) {
                    if ( auto pThis = pThisWeak.lock(); pThis ) {
                        pThis->onHandshake( ec, callback );
                    }
                } ));

        runTimeoutTimer();
    }

    void onHandshake( beast::error_code ec, std::weak_ptr<AbstractAsyncQuery<bool>> callback ) {

        DBG_MAIN_THREAD

        _ASSERT( !m_handshakeReceived )

        m_timeoutTimer.reset();

        auto c = callback.lock();

        if ( !c ) {
            _LOG_WARN( "OnHandshake Https Connection Empty Callback" );
            close();
            return;
        }

        if ( ec ) {
            _LOG_WARN( "OnHandshake Https Connection Error " << ec.message() );
            close();
            c->postReply( false );
            return;
        }

        m_handshakeReceived = true;

        if ( m_ocspVerifiers.empty() ) {
            onExtendedHandshake( callback );
        }
    }

    void onWritten( beast::error_code ec,
                    std::size_t bytes_transferred,
                    std::weak_ptr<AbstractAsyncQuery<bool>> callback ) {

        DBG_MAIN_THREAD

        m_timeoutTimer.reset();

        auto c = callback.lock();

        if ( !c ) {
            _LOG_WARN( "OnWritten Https Connection Empty Callback" );
            close();
            return;
        }

        if ( ec ) {
            _LOG_WARN( "OnWritten Https Connection Error " << ec.message() );
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

        m_timeoutTimer.reset();

        auto c = callback.lock();

        if ( !c ) {
            _LOG_WARN( "OnRead Https Connection Empty Callback" );
            close();
            return;
        }

        if ( ec == http::error::need_buffer ) {
            ec.assign(0, ec.category());
        }

        if( ec ) {
            _LOG_WARN( "OnRead Https Connection Error " << ec.message() );
            close();
            c->postReply( {} );
            return;
        }

        if ( m_res.is_done() || m_res.get().body().size == 0 ) {
            auto readSize = m_readDataBuffer
                    .size() - m_res.get().body().size;
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

        m_timeoutTimer = m_threadManager.startTimer( m_connectionTimeout, [pThisWeak = weak_from_this()] {
            if ( auto pThis = pThisWeak.lock(); pThis ) {
                beast::get_lowest_layer( pThis->m_stream ).cancel();
            }
        } );
    }

    void verifyOCSP( ssl::verify_context& c, std::weak_ptr<AbstractAsyncQuery<bool>> callback ) {

        DBG_MAIN_THREAD

        auto* handle = c.native_handle();

        auto* cert = X509_STORE_CTX_get_current_cert( handle );
        auto* issuer = X509_STORE_CTX_get0_current_issuer( handle );

        if ( cert == issuer ) {
            return;
        }
        auto requestId = utils::generateRandomByteValue<RequestId>();

        auto ocspVerifier = std::make_unique<OCSPVerifier>(
                X509_STORE_CTX_get0_store( handle ),
                X509_STORE_CTX_get1_chain( handle ),
                [=, pThisWeak = weak_from_this()]( CertificateRevocationCheckStatus status ) {
                    if ( auto pThis = pThisWeak.lock(); pThis ) {
                        pThis->onOCSPVerified( requestId, status, callback );
                    }
                },
                m_threadManager,
                m_ocspQueryTimerDelay,
                m_ocspQueryMaxEfforts,
                m_dbgInfo );
        auto [it, success] = m_ocspVerifiers.emplace( requestId, std::move( ocspVerifier ));
        it->second->run( cert, issuer );
    }

    void onOCSPVerified( const RequestId& requestId, CertificateRevocationCheckStatus status, std::weak_ptr<AbstractAsyncQuery<bool>> callback ) {

        DBG_MAIN_THREAD

        auto c = callback.lock();

        if ( !c ) {
            _LOG_WARN( "OnOCSPVerified Https Connection Empty Callback" );
            return;
        }

        auto it = m_ocspVerifiers.find( requestId );

        if ( it == m_ocspVerifiers.end() ) {
            return;
        }

        m_ocspVerifiers.erase( it );

        bool ocspValid = true;

        if ( m_revocationVerificationMode == RevocationVerificationMode::SOFT &&
             status == CertificateRevocationCheckStatus::REVOKED
             or
             m_revocationVerificationMode == RevocationVerificationMode::HARD &&
             status != CertificateRevocationCheckStatus::VALID ) {
            ocspValid = false;
        }


        if ( !ocspValid ) {
            _LOG_WARN( "OnOCSPVerified Https Connection OCSP Invalid" );
            close();
            c->postReply( false );
            return;
        }

        if ( m_ocspVerifiers.empty() && m_handshakeReceived ) {
            onExtendedHandshake( callback );
        }
    }

    void onExtendedHandshake( std::weak_ptr<AbstractAsyncQuery<bool>> callback ) {
        http::async_write( m_stream, m_req,
                           beast::bind_front_handler( [pThisWeak = weak_from_this(), callback]( beast::error_code ec,
                                   std::size_t bytes_transferred ) {
                               if ( auto pThis = pThisWeak.lock(); pThis ) {
                                   pThis->onWritten( ec, bytes_transferred, callback );
                               }
                           } ));

        runTimeoutTimer();
    }

    void onShutdown( beast::error_code ec ) {

        DBG_MAIN_THREAD

        _ASSERT( m_state == ConnectionState::SHUTDOWN );

        if ( ec == net::error::eof ) {

        }

        if ( ec ) {
            // Error Is Normal Here
            _LOG_WARN( "Error During Shutdown: " << ec.message() );
        }

        m_state = ConnectionState::CLOSED;

        m_timeoutTimer.reset();

        beast::get_lowest_layer( m_stream ).close();
    }
};

}