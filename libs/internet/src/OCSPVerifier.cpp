/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "OCSPVerifier.h"

namespace sirius::contract::internet {

OCSPVerifier::OCSPVerifier( X509_STORE* store,
                            stack_st_X509* chain,
                            std::function<void( CertificateRevocationCheckStatus )> asyncCallback,
                            GlobalEnvironment& globalEnvironment,
                            int timerDelayMs,
                            int maxEfforts)
                            : m_store( store )
                            , m_chain( chain )
                            , m_asyncCallback( std::move( asyncCallback ))
                            , m_globalEnvironment( globalEnvironment )
                            , m_timerDelayMs( timerDelayMs )
                            , m_maxEfforts( maxEfforts ) {}

OCSPVerifier::~OCSPVerifier() {
    m_ocspChecker.reset();
    sk_X509_free(m_chain);
    OCSP_REQUEST_free( m_request );
}

void OCSPVerifier::run( X509* cert, X509* issuer ) {
    prepareRequest( cert, issuer );
    prepareURLs( cert );
    sendRequest();
}

void OCSPVerifier::prepareRequest( X509* cert, X509* issuer ) {

    ASSERT( isSingleThread(), m_globalEnvironment.logger() )

    if ( !issuer ) {
        m_globalEnvironment.logger().warn("Certificate Issuer Is Null");
        postReply( CertificateRevocationCheckStatus::UNDEFINED );
        return;
    }

    m_request = OCSP_REQUEST_new();

    if ( !m_request ) {
        m_globalEnvironment.logger().warn("OCSP Request Is Null");
        postReply( CertificateRevocationCheckStatus::UNDEFINED );
        return;
    }

    const EVP_MD* certIdMd = EVP_sha1();
    m_requestId = OCSP_cert_to_id( certIdMd, cert, issuer );

    if ( !m_requestId ) {
        m_globalEnvironment.logger().warn( "Request Id Is Null" );
        postReply( CertificateRevocationCheckStatus::UNDEFINED );
        return;
    }

    if ( !OCSP_request_add0_id( m_request, m_requestId )) {
        m_globalEnvironment.logger().warn( "Could Not Add Id To Request" );
        postReply( CertificateRevocationCheckStatus::UNDEFINED );
        return;
    }
}

void OCSPVerifier::prepareURLs( X509* cert ) {

    ASSERT( isSingleThread(), m_globalEnvironment.logger() )

    STACK_OF( OPENSSL_STRING )* ocsp_list = X509_get1_ocsp( cert );
    for ( int i = 0; i < sk_OPENSSL_STRING_num( ocsp_list ); i++ ) {
        m_urls.emplace_back( sk_OPENSSL_STRING_value( ocsp_list, i ));
    }
    X509_email_free( ocsp_list );
}

void OCSPVerifier::sendRequest() {

    ASSERT( isSingleThread(), m_globalEnvironment.logger() )

    if ( m_urls.empty() ) {
        m_globalEnvironment.logger().warn( "No More URLs To Process" );
        postReply( CertificateRevocationCheckStatus::UNDEFINED );
        return;
    }

    std::string url = std::move( m_urls.front() );
    m_urls.pop_front();

    m_ocspChecker = std::make_unique<OCSPHandler>( std::move( url ), m_request, m_requestId, m_chain, m_store,
                                                   m_globalEnvironment,
                                                   [this]( CertificateRevocationCheckStatus status ) {
        onOCSPChecked( status );
        }, m_timerDelayMs, m_maxEfforts );

    m_ocspChecker->run();
}

void OCSPVerifier::onOCSPChecked( CertificateRevocationCheckStatus status ) {

    ASSERT( isSingleThread(), m_globalEnvironment.logger() )

    ASSERT( m_ocspChecker, m_globalEnvironment.logger() )

    ASSERT( m_revocationStatus == CertificateRevocationCheckStatus::UNDEFINED, m_globalEnvironment.logger() )

    m_ocspChecker.reset();

    if ( status != CertificateRevocationCheckStatus::UNDEFINED ) {
        m_revocationStatus = status;
        postReply( m_revocationStatus );
        return;
    }

    sendRequest();
}

void OCSPVerifier::postReply( CertificateRevocationCheckStatus status ) {
    m_globalEnvironment.threadManager().execute( [=, callback = std::move( m_asyncCallback )] {
        callback( status );
    } );
}

}