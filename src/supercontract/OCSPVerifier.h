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
#include "OCSPHandler.h"

namespace sirius::contract {


class OCSPVerifier {

private:

    // We do not own this object
    X509_STORE* m_store;

    // We DO own these objets
    stack_st_X509*  m_chain;
    OCSP_REQUEST*   m_request;
    OCSP_CERTID*    m_requestId;

    std::deque<std::string> m_urls;

    std::unique_ptr<OCSPHandler> m_ocspChecker;

    std::function<void( CertificateRevocationCheckStatus )> m_callback;

    ThreadManager& m_threadManager;

    CertificateRevocationCheckStatus m_revocationStatus = CertificateRevocationCheckStatus::UNDEFINED;

    const int m_timerDelayMs;
    const int m_maxEfforts;

    DebugInfo m_dbgInfo;

public:

    OCSPVerifier( ThreadManager& threadManager, int timerDelayMs, int maxEfforts, const DebugInfo& debugInfo )
            : m_threadManager( threadManager )
            , m_timerDelayMs( timerDelayMs )
            , m_maxEfforts( maxEfforts )
            , m_dbgInfo( debugInfo )
            {}

    ~OCSPVerifier() {
        sk_X509_free(m_chain);
        OCSP_REQUEST_free( m_request );
        OCSP_CERTID_free( m_requestId );
    }

    void run( X509* cert, X509* issuer ) {
        prepareRequest( cert, issuer );
        sendRequest();
    }

private:

    void prepareRequest( X509* cert, X509* issuer ) {

        DBG_MAIN_THREAD

        if ( !issuer ) {
            m_callback(  CertificateRevocationCheckStatus::UNDEFINED );
            return;
        }

        m_request = OCSP_REQUEST_new();

        if ( !m_request ) {
            m_callback( CertificateRevocationCheckStatus::UNDEFINED );
            return;
        }

        if ( !m_requestId ) {
            m_callback( CertificateRevocationCheckStatus::UNDEFINED );
            return;
        }

        const EVP_MD* certIdMd = EVP_sha1();
        m_requestId = OCSP_cert_to_id( certIdMd, cert, issuer );
        if ( !OCSP_request_add0_id( m_request, m_requestId )) {
            m_callback( CertificateRevocationCheckStatus::UNDEFINED );
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
            m_callback( CertificateRevocationCheckStatus::UNDEFINED );
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
            m_callback( m_revocationStatus );
        }
        else {

        }
    }

};

}