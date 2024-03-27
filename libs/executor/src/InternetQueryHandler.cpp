/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "InternetQueryHandler.h"
#include <internet/InternetUtils.h>

namespace sirius::contract {

InternetQueryHandler::InternetQueryHandler(const CallId& callId,
										   uint64_t height,
                                           uint16_t maxConnections,
                                           ExecutorEnvironment& executorEnvironment,
                                           ContractEnvironment& contractEnvironment)
        : m_executorEnvironment(executorEnvironment)
          , m_contractEnvironment(contractEnvironment)
          , m_callId(callId)
		  , m_height(height)
          , m_maxConnections(maxConnections) {}

void InternetQueryHandler::openConnection(const std::string& url,
                                          bool softRevocationMode,
                                          std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {

    ASSERT(isSingleThread(), m_executorEnvironment.logger())

    ASSERT(!m_asyncQuery, m_executorEnvironment.logger())

    if (m_internetConnections.size() >= m_maxConnections) {
        callback->postReply(
                tl::unexpected<std::error_code>(
                        internet::make_error_code(internet::InternetError::connections_limit_reached)));
        return;
    }

    auto urlDescription = parseURL(url);

    if (!urlDescription) {
        callback->postReply(
                tl::unexpected<std::error_code>(internet::make_error_code(internet::InternetError::invalid_url_error)));
        return;
    }

    auto connectionId = totalConnectionsCreated;
    totalConnectionsCreated++;

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

        auto revocationMode = softRevocationMode ? internet::RevocationVerificationMode::SOFT
                                                 : internet::RevocationVerificationMode::HARD;
        internet::InternetConnection::buildHttpsInternetConnection(m_executorEnvironment.sslContext(),
                                                                   m_executorEnvironment,
                                                                   urlDescription->host,
                                                                   urlDescription->port,
                                                                   urlDescription->target,
                                                                   m_executorEnvironment.executorConfig().getConfigByHeight(m_height).internetBufferSize,
                                                                   m_executorEnvironment.executorConfig().internetConnectionTimeoutMilliseconds(),
                                                                   m_executorEnvironment.executorConfig().ocspQueryTimerMilliseconds(),
                                                                   m_executorEnvironment.executorConfig().ocspQueryMaxEfforts(),
                                                                   revocationMode,
                                                                   connectionCallback);
    } else {
        internet::InternetConnection::buildHttpInternetConnection(m_executorEnvironment,
                                                                  urlDescription->host,
                                                                  urlDescription->port,
                                                                  urlDescription->target,
                                                                  m_executorEnvironment.executorConfig().getConfigByHeight(m_height).internetBufferSize,
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