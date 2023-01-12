/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "InternetQueryHandler.h"
#include <internet/InternetUtils.h>

namespace sirius::contract {

InternetQueryHandler::InternetQueryHandler(const CallId& callId,
                                           ExecutorEnvironment& executorEnvironment,
                                           ContractEnvironment& contractEnvironment)
        : m_executorEnvironment(executorEnvironment)
        , m_contractEnvironment(contractEnvironment)
        , m_callId(callId) {}

void InternetQueryHandler::openConnection(const std::string& url,
                                          std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto urlDescription = parseURL(url);

    if (!urlDescription) {
        m_executorEnvironment.logger().warn("Invalid URL \"{}\" at Contract Call {}", url, m_callId);
        callback->postReply(
                tl::unexpected<std::error_code>(internet::make_error_code(internet::InternetError::invalid_url_error)));
        return;
    }

    auto connectionId = totalConnectionsCreated;
    totalConnectionsCreated++;

    m_executorEnvironment.logger().info("Contract call {} requested to open connection to {}, connection id: {}",
                                        m_callId, url, connectionId);

    auto[query, connectionCallback] = createAsyncQuery<internet::InternetConnection>(
            [this, callback, connectionId](auto&& connection) {
                // If the callback is executed, 'this' will always be alive
                if (!connection) {
                    callback->postReply(tl::unexpected<std::error_code>(connection.error()));
                } else {
                    ASSERT(!m_internetConnections.contains(connectionId), m_executorEnvironment.logger())
                    m_internetConnections.emplace(connectionId, std::move(*connection));
                    callback->postReply(connectionId);
                }
                m_asyncQuery.reset();
            },
            [callback] {
                callback->postReply(tl::unexpected<std::error_code>(
                        make_error_code(internet::InternetError::internet_unavailable)));
            },
            m_executorEnvironment, true, false);

    m_asyncQuery = std::move(query);

    if (urlDescription->ssl) {
        internet::InternetConnection::buildHttpsInternetConnection(m_executorEnvironment.sslContext(),
                                                                   m_executorEnvironment,
                                                                   urlDescription->host,
                                                                   urlDescription->port,
                                                                   urlDescription->target,
                                                                   m_executorEnvironment.executorConfig().internetBufferSize(),
                                                                   m_executorEnvironment.executorConfig().internetConnectionTimeoutMilliseconds(),
                                                                   m_executorEnvironment.executorConfig().ocspQueryTimerMilliseconds(),
                                                                   m_executorEnvironment.executorConfig().ocspQueryMaxEfforts(),
                                                                   internet::RevocationVerificationMode::SOFT,
                                                                   connectionCallback);
    } else {
        internet::InternetConnection::buildHttpInternetConnection(m_executorEnvironment,
                                                                  urlDescription->host,
                                                                  urlDescription->port,
                                                                  urlDescription->target,
                                                                  m_executorEnvironment.executorConfig().internetBufferSize(),
                                                                  m_executorEnvironment.executorConfig().internetConnectionTimeoutMilliseconds(),
                                                                  connectionCallback);
    }
}

void InternetQueryHandler::read(uint64_t connectionId,
                                std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    auto connectionIt = m_internetConnections.find(connectionId);
    if (connectionIt == m_internetConnections.end()) {
        callback->postReply(tl::unexpected<std::error_code>(
                internet::make_error_code(internet::InternetError::invalid_resource_error)));
        return;
    }

    m_executorEnvironment.logger().info("Contract call {} requested to read from internet connection {}",
                                        m_callId, connectionId);

    auto[query, readCallback] = createAsyncQuery<std::vector<uint8_t>>(
            [this, callback](auto&& data) {
                // If the callback is executed, 'this' will always be alive
                callback->postReply(std::forward<decltype(data)>(data));
                m_asyncQuery.reset();
            },
            [callback] {
                callback->postReply(tl::unexpected<std::error_code>(
                        make_error_code(internet::InternetError::internet_unavailable)));
            },
            m_executorEnvironment, true, false);
    m_asyncQuery = std::move(query);
    connectionIt->second.read(readCallback);
}

void InternetQueryHandler::closeConnection(uint64_t connectionId, std::shared_ptr<AsyncQueryCallback<void>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    m_executorEnvironment.logger().info("Contract call {} requested to close internet connection {}",
                                        m_callId, connectionId);

    auto connectionIt = m_internetConnections.find(connectionId);
    if (connectionIt == m_internetConnections.end()) {
        callback->postReply(
                tl::make_unexpected(internet::make_error_code(internet::InternetError::invalid_resource_error)));
        return;
    }

    m_internetConnections.erase(connectionIt);
    callback->postReply(expected<void>());
}

}