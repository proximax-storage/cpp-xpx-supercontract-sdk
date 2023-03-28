/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "OCSPHandler.h"
#include <openssl/ssl.h>
#include <openssl/bio.h>

namespace sirius::contract::internet {

class OCSPResponseException : public std::runtime_error {
public :

    explicit OCSPResponseException( const std::string& what )
    : std::runtime_error( what )
    {}

};

OCSPHandler::OCSPHandler(
        std::string&& url,
        OCSP_REQUEST* request,
        OCSP_CERTID* requestId,
        stack_st_X509* chain,
        X509_STORE* store,
        GlobalEnvironment& globalEnvironment,
        std::function<void( CertificateRevocationCheckStatus )> callback,
        int timerDelayMs,
        int maxEfforts )
        : m_request( request )
        , m_requestId( requestId )
        , m_chain( chain )
        , m_store( store )
        , m_globalEnvironment( globalEnvironment )
        , m_timerDelayMs( timerDelayMs )
        , m_maxEfforts( maxEfforts )
        , m_effortsLeft( maxEfforts )
        , m_callback( std::move( callback )){
    OCSP_parse_url( url.c_str(), &m_host, &m_port, &m_path, &m_ssl );
}

OCSPHandler::~OCSPHandler() {

    ASSERT( isSingleThread(), m_globalEnvironment.logger() )

    // After the timer is reset we are sure that no asynchronous operation will be called
    // And so there will not be asynchronous object access
    m_retryTimer.cancel();

    if ( m_socketBio ) {
        BIO_set_close( m_socketBio, BIO_CLOSE );
        BIO_free_all( m_socketBio );
    }

    OCSP_RESPONSE_free( m_response );
    OCSP_REQ_CTX_free( m_ctx );
    OPENSSL_free( m_host );
    OPENSSL_free( m_path );
    OPENSSL_free( m_port );
}

void OCSPHandler::run() {

    ASSERT( isSingleThread(), m_globalEnvironment.logger() )

    sendRequest();
}

void OCSPHandler::sendRequest() {

    ASSERT( isSingleThread(), m_globalEnvironment.logger() )

    ASSERT( m_host, m_globalEnvironment.logger() )
    ASSERT( m_port, m_globalEnvironment.logger() )
    ASSERT( m_path, m_globalEnvironment.logger() )

    if ( m_ssl != 0 ) {
        m_globalEnvironment.logger().debug("SSL in OCSP query");
        m_callback( CertificateRevocationCheckStatus::UNDEFINED );
        return;
    }

    m_socketBio = BIO_new_connect( m_host );

    if ( !m_socketBio ) {
        m_globalEnvironment.logger().debug( "Socket BIO creation failed" );
        m_callback( CertificateRevocationCheckStatus::UNDEFINED );
        return;
    }

    BIO_set_conn_port( m_socketBio, m_port );
    BIO_set_nbio( m_socketBio, 1 );

    m_ctx = OCSP_sendreq_new( m_socketBio, m_path, nullptr, -1 );

    if ( !m_ctx ) {
        m_globalEnvironment.logger().debug( "OCSP context creation failed" );
        m_callback( CertificateRevocationCheckStatus::UNDEFINED );
        return;
    }

    if ( !OCSP_REQ_CTX_add1_header( m_ctx, "Host", m_host ) ) {
        m_globalEnvironment.logger().debug( "OCSP header add failed" );
        m_callback( CertificateRevocationCheckStatus::UNDEFINED );
        return;
    }

    if ( !OCSP_REQ_CTX_set1_req( m_ctx, m_request ) ) {
        m_globalEnvironment.logger().debug( "OCSP context add request failed" );
        m_callback( CertificateRevocationCheckStatus::UNDEFINED );
        return;
    }

    connect();
}

void OCSPHandler::connect() {

    ASSERT( isSingleThread(), m_globalEnvironment.logger() )

    auto rv = BIO_do_connect( m_socketBio );

    if ( rv > 0 ) {
        // Successfully connected
        m_effortsLeft = m_maxEfforts;
        read();
    }
    else if ( BIO_should_retry( m_socketBio ) && m_effortsLeft > 0 ) {
        m_effortsLeft--;
        m_retryTimer = m_globalEnvironment.threadManager().startTimer( m_timerDelayMs, [this] {
            connect();
        } );
    }
    else {
        // Real error has occurred
        m_globalEnvironment.logger().debug( "OCSP connection to {}:{} failed", m_host, m_port);
        m_callback( CertificateRevocationCheckStatus::UNDEFINED );
    }
}

void OCSPHandler::read() {

    ASSERT( isSingleThread(), m_globalEnvironment.logger() )

    ASSERT( m_ctx, m_globalEnvironment.logger() )

    auto rv = OCSP_sendreq_nbio( &m_response, m_ctx );
    if ( rv > 0 ) {
        onResponseReceived();
    }
    else if ( rv < 0 && m_effortsLeft > 0 ) {
        m_effortsLeft--;
        m_retryTimer = m_globalEnvironment.threadManager().startTimer( m_timerDelayMs, [this] {
            read();
        });
    }
    else {
        m_globalEnvironment.logger().debug("OCSP read response from {}:{} failed", m_host, m_port);
        m_callback( CertificateRevocationCheckStatus::UNDEFINED );
        return;
    }
}

void OCSPHandler::onResponseReceived() {

    ASSERT( isSingleThread(), m_globalEnvironment.logger() )

    OCSP_BASICRESP* basicResponse = nullptr;
    ASN1_GENERALIZEDTIME* producedAt = nullptr;
    ASN1_GENERALIZEDTIME* thisUpdate = nullptr;
    ASN1_GENERALIZEDTIME* nextUpdate = nullptr;
    CertificateRevocationCheckStatus revocationStatus;

    try {
        if ( !m_response ) {
            throw OCSPResponseException( "OCSP Response Is Null" );
        }

        int responder_status = OCSP_response_status( m_response );

        if ( responder_status != OCSP_RESPONSE_STATUS_SUCCESSFUL )  {
            throw OCSPResponseException( "OCSP Response Unsuccessful" );
        }

        basicResponse = OCSP_response_get1_basic( m_response );

        if ( !basicResponse ) {
            throw OCSPResponseException( "OCSP Basic Response Is Null" );
        }

        int status;
        int reason;
        bool foundId = OCSP_resp_find_status( basicResponse, m_requestId, &status, &reason, &producedAt, &thisUpdate, &nextUpdate );

        if ( !foundId ) {
            throw OCSPResponseException( "OCSP Response Id Not Found" );
        }

        if ( !OCSP_basic_verify( basicResponse, m_chain, m_store, 0 ) ) {
            throw OCSPResponseException( "OCSP Response Basic Verify Not Passed" );
        }

        if ( status == V_OCSP_CERTSTATUS_GOOD ) {
            revocationStatus = CertificateRevocationCheckStatus::VALID;
        }
        else if ( status == V_OCSP_CERTSTATUS_REVOKED ) {
            revocationStatus = CertificateRevocationCheckStatus::REVOKED;
        }
        else {
            throw OCSPResponseException( "OCSP Response Invalid Status" );
        }
    }
    catch ( const OCSPResponseException& ex ) {
        m_globalEnvironment.logger().debug("OCSP parse response failed: {}", ex.what());
        revocationStatus = CertificateRevocationCheckStatus::UNDEFINED;
    }
    OCSP_BASICRESP_free( basicResponse );
    m_callback(revocationStatus);
}

}