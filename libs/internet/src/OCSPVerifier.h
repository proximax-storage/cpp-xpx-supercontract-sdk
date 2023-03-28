/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <deque>

#include <openssl/bio.h>
#include <openssl/ocsp.h>

#include "OCSPHandler.h"

#include <common/ThreadManager.h>

namespace sirius::contract::internet {

class OCSPVerifier: private SingleThread {

private:

    // We do not own this object
    X509_STORE* m_store;

    // We DO own these objets
    stack_st_X509*  m_chain;
    OCSP_REQUEST*   m_request = nullptr;

    // Is Not Freed Up because of calling OCSP_request_add0_id
    OCSP_CERTID*    m_requestId = nullptr;

    std::deque<std::string> m_urls;

    std::unique_ptr<OCSPHandler> m_ocspChecker;

    // The callback is ALWAYS executed asynchronously
    // After calling the callback one can not know
    // whether the object exists anymore
    std::function<void( CertificateRevocationCheckStatus )> m_asyncCallback;

    GlobalEnvironment& m_globalEnvironment;

    CertificateRevocationCheckStatus m_revocationStatus = CertificateRevocationCheckStatus::UNDEFINED;

    const int m_timerDelayMs;
    const int m_maxEfforts;

public:

    OCSPVerifier( X509_STORE* store,
                  stack_st_X509* chain,
                  std::function<void( CertificateRevocationCheckStatus )> asyncCallback,
                  GlobalEnvironment& globalEnvironment,
                  int timerDelayMs,
                  int maxEfforts );

    OCSPVerifier( const OCSPVerifier& ) = delete;

    OCSPVerifier( OCSPVerifier&& ) = delete;

    OCSPVerifier& operator=( OCSPVerifier&& ) = delete;

    OCSPVerifier& operator=( const OCSPVerifier& ) = delete;

    ~OCSPVerifier();

    void run( X509* cert, X509* issuer );

private:

    void prepareRequest( X509* cert, X509* issuer );

    void prepareURLs( X509* cert );

    void sendRequest();

    void onOCSPChecked( CertificateRevocationCheckStatus status );

    void postReply( CertificateRevocationCheckStatus status );

};

}