/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <memory>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/impl/context.ipp>

#include "InternetConnection.h"
#include "../../src/HttpInternetResource.h"
#include "../../src/HttpsInternetResource.h"

#include "supercontract/AsyncQuery.h"
#include "supercontract/SingleThread.h"
#include "supercontract/GlobalEnvironment.h"


namespace sirius::contract::internet {

class InternetConnection:
        private SingleThread {

    GlobalEnvironment&            m_environment;
    InternetResourceContainer     m_resource;

private:

    InternetConnection( GlobalEnvironment& environment,
                        InternetResourceContainer&& resource );

public:

    ~InternetConnection();

    InternetConnection( const InternetConnection& ) = delete;
    InternetConnection( InternetConnection&& other ) noexcept;
    InternetConnection& operator=( const InternetConnection& ) = delete;
    InternetConnection& operator=( InternetConnection&& other )  noexcept;

    static void buildHttpInternetConnection( GlobalEnvironment& globalEnvironment,
                                             const std::string& host,
                                             const std::string& port,
                                             const std::string& target,
                                             int bufferSize,
                                             int timeout,
                                             const std::shared_ptr<AsyncCallback<std::optional<InternetConnection>>>&);

    static void buildHttpsInternetConnection( boost::asio::ssl::context& ctx,
                                              GlobalEnvironment& globalEnvironment,
                                              const std::string& host,
                                              const std::string& port,
                                              const std::string& target,
                                              int bufferSize,
                                              int connectionTimeout,
                                              int ocspQueryTimerDelay,
                                              int ocspQueryMaxEfforts,
                                              RevocationVerificationMode mode,
                                              const std::shared_ptr<AsyncCallback<std::optional<InternetConnection>>>&);

};
}