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
    VALID, REVOKED, UNDEFINED
};

class OCSPResponseException : public std::runtime_error {
public:

    OCSPResponseException( const std::string& what )
    : std::runtime_error( what )
    {}

};

class OCSPHandler {

private:

    BIO*            m_socketBio = nullptr;
    OCSP_REQ_CTX*   m_ctx = nullptr;
    OCSP_RESPONSE*  m_response = nullptr;

    // We do not own this object
    OCSP_REQUEST*   m_request;
    OCSP_CERTID*   m_requestId;
    stack_st_X509*  m_chain;
    X509_STORE*     m_store;

    char*   m_host = nullptr;
    char*   m_port = nullptr;
    char*   m_path = nullptr;
    int     m_ssl = 0;

    ThreadManager& m_threadManager;

    std::optional<boost::asio::high_resolution_timer> m_retryTimer;
    const int m_timerDelayMs;
    const int m_maxEfforts;
    int m_effortsLeft;

    std::function<void( CertificateRevocationCheckStatus status )> m_callback;

    DebugInfo m_dbgInfo;

public:

    OCSPHandler( std::string&& url,
                 OCSP_REQUEST* request,
                 OCSP_CERTID* requestId,
                 stack_st_X509* chain,
                 X509_STORE* store,
                 ThreadManager& threadManager,
                 std::function<void( CertificateRevocationCheckStatus status )> callback,
                 int timerDelayMs,
                 int maxEfforts,
                 const DebugInfo& debugInfo )
            : m_request( request )
            , m_requestId( requestId )
            , m_chain( chain )
            , m_store( store )
            , m_threadManager( threadManager )
            , m_callback( std::move(callback) )
            , m_timerDelayMs( timerDelayMs )
            , m_maxEfforts( maxEfforts )
            , m_effortsLeft( maxEfforts )
            , m_dbgInfo( debugInfo ) {
        OCSP_parse_url(url.c_str(), &m_host, &m_port, &m_path, &m_ssl );
    }

    void run() {
        sendRequest();
    }

    ~OCSPHandler() {

        DBG_MAIN_THREAD

        m_retryTimer.reset();

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

private:

    void sendRequest() {

        DBG_MAIN_THREAD

        _ASSERT( m_host )
        _ASSERT( m_port )
        _ASSERT( m_path )

        if ( m_ssl != 0 ) {
            m_callback( CertificateRevocationCheckStatus::UNDEFINED );
            return;
        }

        m_socketBio = BIO_new_connect( m_host );

        if ( !m_socketBio ) {
            m_callback( CertificateRevocationCheckStatus::UNDEFINED );
            return;
        }

        BIO_set_conn_port( m_socketBio, m_port );
        BIO_set_nbio( m_socketBio, 1 );

        m_ctx = OCSP_sendreq_new( m_socketBio, m_path, nullptr, -1 );

        if ( !m_ctx ) {
            m_callback( CertificateRevocationCheckStatus::UNDEFINED );
            return;
        }

        if ( !OCSP_REQ_CTX_add1_header( m_ctx, "Host", m_host ) ) {
            m_callback( CertificateRevocationCheckStatus::UNDEFINED );
            return;
        }

        if ( !OCSP_REQ_CTX_set1_req( m_ctx, m_request ) ) {
            m_callback( CertificateRevocationCheckStatus::UNDEFINED );
            return;
        }

        connect();
    }


    void connect() {

        DBG_MAIN_THREAD

        auto rv = BIO_do_connect( m_socketBio );

        if ( rv > 0 ) {
            // Successfully connected
            m_effortsLeft = m_maxEfforts;
            read();
        }
        else if ( rv < 0 && m_effortsLeft > 0 ) {
            m_effortsLeft--;
            m_retryTimer = m_threadManager.startTimer( m_timerDelayMs, [this] {
                connect();
            } );
        }
        else {
            // Real error has occurred
            m_callback( CertificateRevocationCheckStatus::UNDEFINED );
        }
    }

    void read() {

        DBG_MAIN_THREAD

        _ASSERT( m_ctx )

        auto rv = OCSP_sendreq_nbio( &m_response, m_ctx );
        if ( rv > 0 ) {
            onResponseReceived();
        }
        else if ( rv < 0 && m_effortsLeft > 0 ) {
            m_effortsLeft--;
            m_retryTimer = m_threadManager.startTimer( m_timerDelayMs, [this] {
                read();
            });
        }
        else {
            m_callback( CertificateRevocationCheckStatus::UNDEFINED );
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
            revocationStatus = CertificateRevocationCheckStatus::UNDEFINED;
        }
        OCSP_BASICRESP_free( basicResponse );
        m_callback(revocationStatus);
    }
};

}