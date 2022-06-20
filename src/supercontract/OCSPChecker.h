/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/x509_vfy.h>
#include <openssl/ssl.h>
#include <openssl/ocsp.h>

namespace sirius::contract {

enum class CertificateRevocationCheckStatus {
    VALID, REVOKED, UNKNOWN, ERROR
};

class OCSPCheckException : public std::runtime_error {
public:

    OCSPCheckException( const std::string& what )
    : std::runtime_error( what )
    {}

};

class OCSPChecker {

private:

    BIO*            m_socketBio = nullptr;
    OCSP_REQ_CTX*   m_ctx = nullptr;
    OCSP_RESPONSE*  m_response = nullptr;
    OCSP_REQUEST*   m_request;

    char*   m_host = nullptr;
    char*   m_port = nullptr;
    char*   m_path = nullptr;
    int     m_ssl = 0;

    ThreadManager& m_threadManager;

    std::optional<boost::asio::high_resolution_timer> m_retryTimer;

    std::function<void( CertificateRevocationCheckStatus status )> m_callback;

    DebugInfo m_dbgInfo;

public:

    OCSPChecker( std::string&& url,
                 OCSP_REQUEST* request,
                 ThreadManager& threadManager,
                 std::function<void( CertificateRevocationCheckStatus status )> callback,
                 const DebugInfo& debugInfo )
            : m_request( request )
            , m_threadManager( threadManager )
            , m_callback( std::move(callback) )
            , m_dbgInfo( debugInfo ) {
        OCSP_parse_url(url.c_str(), &m_host, &m_port, &m_path, &m_ssl );
    }

    void run() {
        sendRequest();
    }

private:

    void close() {

        DBG_MAIN_THREAD

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

    void sendRequest() {

        DBG_MAIN_THREAD

        _ASSERT( m_host )
        _ASSERT( m_port )
        _ASSERT( m_path )

        if ( m_ssl != 0 ) {
            m_callback( CertificateRevocationCheckStatus::ERROR );
            return;
        }

        m_socketBio = BIO_new_connect( m_host );

        if ( !m_socketBio ) {
            m_callback( CertificateRevocationCheckStatus::ERROR );
            return;
        }

        BIO_set_conn_port( m_socketBio, m_port );
        BIO_set_nbio( m_socketBio, 1 );

        m_ctx = OCSP_sendreq_new( m_socketBio, m_path, nullptr, -1 );

        if ( !m_ctx ) {
            m_callback( CertificateRevocationCheckStatus::ERROR );
            return;
        }

        if ( !OCSP_REQ_CTX_add1_header( m_ctx, "Host", m_host ) ) {
            m_callback( CertificateRevocationCheckStatus::ERROR );
            return;
        }

        if ( !OCSP_REQ_CTX_set1_req( m_ctx, m_request ) ) {
            m_callback( CertificateRevocationCheckStatus::ERROR );
            return;
        }

        connect();
    }


    void connect() {

        DBG_MAIN_THREAD

        auto rv = BIO_do_connect( m_socketBio );

        if ( rv > 0 ) {
            read();
            // Successfully connected
        }
        else if ( rv < 0 ) {
            m_retryTimer = m_threadManager.startTimer( 500, [this] {
                connect();
            } );
        }
        else {
            // Real error has occurred
            m_callback( CertificateRevocationCheckStatus::ERROR );
        }
    }

    void read() {

        DBG_MAIN_THREAD

        _ASSERT( m_ctx )

        auto rv = OCSP_sendreq_nbio( &m_response, m_ctx );
        if ( rv > 0 ) {
            m_retryTimer.reset();
            onResponseReceived();
        }
        else if ( rv < 0 ) {
            m_retryTimer = m_threadManager.startTimer(500, [this] {
                read();
            });
        }
        else {
            m_callback( CertificateRevocationCheckStatus::ERROR );
            return;
        }
    }

    void onResponseReceived() {

        DBG_MAIN_THREAD

        OCSP_BASICRESP* basicResponse = nullptr;
        ASN1_GENERALIZEDTIME* producedAt = nullptr;
        ASN1_GENERALIZEDTIME* thisUpdate = nullptr;
        ASN1_GENERALIZEDTIME* nextUpdate = nullptr;
        CertificateRevocationCheckStatus revocationStatus;

        try {
            if ( !m_response ) {
                throw OCSPCheckException( "OCSP Response Is Null" );
            }

            int responder_status = OCSP_response_status( m_response );

            if ( responder_status != OCSP_RESPONSE_STATUS_SUCCESSFUL )  {
                throw OCSPCheckException( "OCSP Response Unsuccessful" );
            }

            basicResponse = OCSP_response_get1_basic( m_response );

            if ( !basicResponse ) {
                throw OCSPCheckException( "OCSP Basic Response Is Null" );
            }

            int rc, reason, ssl, status;
            bool foundId = OCSP_resp_find_status( basicResponse, id, &status, &reason, &producedAt, &thisUpdate,
                                                  &nextUpdate );

            if ( !foundId ) {
                throw OCSPCheckException( "OCSP Response Id Not Found" );
            }

            if ( status == V_OCSP_CERTSTATUS_GOOD ) {
                revocationStatus = CertificateRevocationCheckStatus::VALID;
            }
            else if ( status == V_OCSP_CERTSTATUS_REVOKED ) {
                revocationStatus = CertificateRevocationCheckStatus::REVOKED;
            }
            else {
                revocationStatus = CertificateRevocationCheckStatus::UNKNOWN;
            }
        }
        catch ( const OCSPCheckException& ex ) {
            revocationStatus = CertificateRevocationCheckStatus::ERROR;
        }
        OCSP_BASICRESP_free( basicResponse );
        ASN1_GENERALIZEDTIME_free( producedAt );
        ASN1_GENERALIZEDTIME_free( thisUpdate );
        ASN1_GENERALIZEDTIME_free( nextUpdate );
        m_callback(revocationStatus);
    }
};

}