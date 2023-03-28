/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <common/GlobalEnvironment.h>
#include <common/SingleThread.h>

#include <openssl/ocsp.h>

namespace sirius::contract::internet {

enum class CertificateRevocationCheckStatus {
    VALID, REVOKED, UNDEFINED
};

class OCSPHandler: private SingleThread {

private:

    BIO*            m_socketBio = nullptr;
    OCSP_REQ_CTX*   m_ctx = nullptr;
    OCSP_RESPONSE*  m_response = nullptr;

    // We do not own this object
    OCSP_REQUEST*   m_request;
    OCSP_CERTID*    m_requestId;
    stack_st_X509*  m_chain;
    X509_STORE*     m_store;

    char*   m_host = nullptr;
    char*   m_port = nullptr;
    char*   m_path = nullptr;
    int     m_ssl = 0;

    GlobalEnvironment& m_globalEnvironment;

    Timer m_retryTimer;
    const int m_timerDelayMs;
    const int m_maxEfforts;
    int m_effortsLeft;

    // The callback might be both synchronous and asynchronous
    // After calling the callback one can not know
    // whether the object exists anymore
    std::function<void( CertificateRevocationCheckStatus status )> m_callback;

public:

    OCSPHandler( std::string&& url,
                 OCSP_REQUEST* request,
                 OCSP_CERTID* requestId,
                 stack_st_X509* chain,
                 X509_STORE* store,
                 GlobalEnvironment& globalEnvironment,
                 std::function<void( CertificateRevocationCheckStatus status )> callback,
                 int timerDelayMs,
                 int maxEfforts );

    OCSPHandler( const OCSPHandler& ) = delete;

    OCSPHandler( OCSPHandler&& ) = delete;

    OCSPHandler& operator=( OCSPHandler&& ) = delete;

    OCSPHandler& operator=( const OCSPHandler& ) = delete;

    void run();

    ~OCSPHandler();

private:

    void sendRequest();

    void connect();

    void read();

    void onResponseReceived();
};

}