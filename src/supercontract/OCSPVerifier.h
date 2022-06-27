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
#include <deque>

#include "contract/ThreadManager.h"
#include "supercontract/DebugInfo.h"
#include "supercontract/log.h"
#include "OCSPHandler.h"

namespace sirius::contract {


class OCSPVerifier {

private:

    // We do not own this object
    X509_STORE* m_store;

    // We DO own these objets
    stack_st_X509*  m_chain;
    OCSP_REQUEST*   m_request = nullptr;

    // Is Not Freed Up because of calling OCSP_request_add0_id
    OCSP_CERTID*    m_requestId;

    std::deque<std::string> m_urls;

    std::unique_ptr<OCSPHandler> m_ocspChecker;

    // The callback is ALWAYS executed asynchronously
    // After calling the callback one can not know
    // whether the object exists anymore
    std::function<void( CertificateRevocationCheckStatus )> m_asyncCallback;

    ThreadManager& m_threadManager;

    CertificateRevocationCheckStatus m_revocationStatus = CertificateRevocationCheckStatus::UNDEFINED;

    const int m_timerDelayMs;
    const int m_maxEfforts;

    DebugInfo m_dbgInfo;

public:

    OCSPVerifier( X509_STORE* store,
                  stack_st_X509* chain,
                  std::function<void( CertificateRevocationCheckStatus )> asyncCallback,
                  ThreadManager& threadManager, int timerDelayMs,
                  int maxEfforts, const DebugInfo& debugInfo )
            : m_store( store )
            , m_chain( chain )
            , m_asyncCallback( std::move( asyncCallback) )
            , m_threadManager( threadManager )
            , m_timerDelayMs( timerDelayMs )
            , m_maxEfforts( maxEfforts )
            , m_dbgInfo( debugInfo )
            {}

    OCSPVerifier( const OCSPVerifier& ) = delete;

    OCSPVerifier( OCSPVerifier&& ) = delete;

    OCSPVerifier& operator=( OCSPVerifier&& ) = delete;

    OCSPVerifier& operator=( const OCSPVerifier& ) = delete;

    ~OCSPVerifier() {
        m_ocspChecker.reset();
        sk_X509_free(m_chain);
        OCSP_REQUEST_free( m_request );
    }

    void run( X509* cert, X509* issuer ) {
        prepareRequest( cert, issuer );
        prepareURLs( cert );
        sendRequest();
    }

private:

    void prepareRequest( X509* cert, X509* issuer ) {

        DBG_MAIN_THREAD

        if ( !issuer ) {
            _LOG_WARN( "Certificate Issuer Is Null" );
            postReply( CertificateRevocationCheckStatus::UNDEFINED );
            return;
        }

        m_request = OCSP_REQUEST_new();

        if ( !m_request ) {
            _LOG_WARN( "Request Is Null" );
            postReply( CertificateRevocationCheckStatus::UNDEFINED );
            return;
        }

        const EVP_MD* certIdMd = EVP_sha1();
        m_requestId = OCSP_cert_to_id( certIdMd, cert, issuer );

        if ( !m_requestId ) {
            _LOG_WARN( "Request Id Is Null" );
            postReply( CertificateRevocationCheckStatus::UNDEFINED );
            return;
        }

        if ( !OCSP_request_add0_id( m_request, m_requestId )) {
            _LOG_WARN( "Could Not Add Id To Request" );
            postReply( CertificateRevocationCheckStatus::UNDEFINED );
            return;
        }
    }

    void prepareURLs( X509* cert ) {

        DBG_MAIN_THREAD

        STACK_OF( OPENSSL_STRING )* ocsp_list = X509_get1_ocsp( cert );
        for ( int i = 0; i < sk_OPENSSL_STRING_num( ocsp_list ); i++ ) {
            m_urls.emplace_back( sk_OPENSSL_STRING_value( ocsp_list, i ));
        }
        X509_email_free( ocsp_list );
    }

    void sendRequest() {

        DBG_MAIN_THREAD

        if ( m_urls.empty() ) {
            _LOG_WARN( "No More URLs To Process" );
            postReply( CertificateRevocationCheckStatus::UNDEFINED );
            return;
        }

        std::string url = std::move( m_urls.front() );
        m_urls.pop_front();

        m_ocspChecker = std::make_unique<OCSPHandler>( std::move( url ), m_request, m_requestId, m_chain, m_store,
                                                       m_threadManager,
                                                       [this]( CertificateRevocationCheckStatus status ) {
                                                           onOCSPChecked( status );
                                                       }, m_timerDelayMs, m_maxEfforts, m_dbgInfo );

        m_ocspChecker->run();
    }

    void onOCSPChecked( CertificateRevocationCheckStatus status ) {

        DBG_MAIN_THREAD

        _ASSERT( m_ocspChecker )

        _ASSERT( m_revocationStatus == CertificateRevocationCheckStatus::UNDEFINED )

        m_ocspChecker.reset();

        if ( status != CertificateRevocationCheckStatus::UNDEFINED ) {
            m_revocationStatus = status;
            postReply( m_revocationStatus );
            return;
        }

        sendRequest();
    }

    void postReply( CertificateRevocationCheckStatus status ) {
        m_threadManager.execute( [=, callback = std::move( m_asyncCallback )] {
            callback( status );
        } );
    }

};

}