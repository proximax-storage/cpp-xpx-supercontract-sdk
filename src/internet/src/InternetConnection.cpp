/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "internet/InternetConnection.h"

namespace sirius::contract::internet {

InternetConnection::InternetConnection( GlobalEnvironment& environment,
                                        InternetResourceContainer&& resource )
        : m_environment( environment )
        , m_resource( std::move( resource )) {

}

InternetConnection::~InternetConnection() {

    ASSERT( isSingleThread(), m_environment.logger() )

    if (m_resource) {
        m_resource->close();
    }
}

InternetConnection::InternetConnection( InternetConnection&& other ) noexcept
        : InternetConnection(other.m_environment, std::move(other.m_resource)) {
}

InternetConnection& InternetConnection::operator=( InternetConnection&& other ) noexcept {

    ASSERT( isSingleThread(), m_environment.logger() )

    m_environment = other.m_environment;
    m_resource = std::move( other.m_resource );
    return *this;
};

void InternetConnection::read(std::shared_ptr<AsyncCallback<std::optional<std::vector<uint8_t>>>> callback) {
    ASSERT(isSingleThread(), m_environment.logger());

    ASSERT(m_resource, m_environment.logger());

    m_resource->read(callback);
}

void InternetConnection::buildHttpInternetConnection( GlobalEnvironment& globalEnvironment, const std::string& host,
                                                      const std::string& port, const std::string& target,
                                                      int bufferSize, int timeout,
                                                      const std::shared_ptr<AsyncCallback<std::optional<InternetConnection>>>& callback ) {

    auto resource = std::make_shared<HttpInternetResource>( globalEnvironment, host, port, target, bufferSize,
                                                            timeout );

    auto openCallback = createAsyncQueryHandler<InternetResourceContainer>(
            [&globalEnvironment, callback]( InternetResourceContainer&& resource ) {
                if ( !resource ) {
                    callback->postReply({});
                    return;
                }
                InternetConnection connection( globalEnvironment, std::move( resource ));
                callback->postReply( std::move( connection ) );
            }, [] {}, globalEnvironment );
    resource->open( openCallback );
}

void InternetConnection::buildHttpsInternetConnection( ssl::context& ctx,
                                                       GlobalEnvironment& globalEnvironment,
                                                       const std::string& host,
                                                       const std::string& port,
                                                       const std::string& target,
                                                       int bufferSize,
                                                       int connectionTimeout,
                                                       int ocspQueryTimerDelay,
                                                       int ocspQueryMaxEfforts,
                                                       RevocationVerificationMode mode,
                                                       const std::shared_ptr<AsyncCallback<std::optional<InternetConnection>>>& callback ) {
    auto resource = std::make_shared<HttpsInternetResource>( ctx,
                                                             globalEnvironment,
                                                             host,
                                                             port,
                                                             target,
                                                             bufferSize,
                                                             connectionTimeout,
                                                             ocspQueryTimerDelay,
                                                             ocspQueryMaxEfforts,
                                                             mode);

    auto openCallback = createAsyncQueryHandler<InternetResourceContainer>(
            [&globalEnvironment, callback]( InternetResourceContainer&& resource ) {
                if ( !resource ) {
                    callback->postReply({});
                    return;
                }
                globalEnvironment.logger().info("I am here");
                InternetConnection connection( globalEnvironment, std::move( resource ));
                callback->postReply( std::move( connection ) );
                }, [] {}, globalEnvironment );
    resource->open( openCallback );
}
}