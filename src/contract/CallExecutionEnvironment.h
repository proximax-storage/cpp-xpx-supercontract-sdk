/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/Identifiers.h"

#include "supercontract/AsyncQuery.h"
#include "internet/HttpInternetConnection.h"
#include "internet/HttpsInternetConnection.h"
#include "internet/InternetUtils.h"
#include "VirtualMachineInternetQueryHandler.h"
#include "VirtualMachineBlockchainQueryHandler.h"

namespace sirius::contract {

class CallExecutionEnvironment
        : public VirtualMachineInternetQueryHandler
        , public VirtualMachineBlockchainQueryHandler {

private:

    ExecutorEnvironment& m_executorEnvironment;
    ContractEnvironment& m_contractEnvironment;

    const CallRequest m_callRequest;

    std::optional<CallExecutionResult> m_callExecutionResult;

    std::shared_ptr<AsyncQuery> m_asyncQuery;

    uint64_t totalConnectionsCreated = 0;
    std::map<uint64_t, std::unique_ptr<internet::InternetConnection>> m_internetConnections;

    bool m_terminated = false;

    const DebugInfo m_dbgInfo;

public:

    CallExecutionEnvironment( const CallRequest& request,
                              ExecutorEnvironment& executorEnvironment,
                              ContractEnvironment& contractEnvironment,
                              const DebugInfo& debugInfo )
            : m_callRequest( request ), m_executorEnvironment( executorEnvironment ),
              m_contractEnvironment( contractEnvironment ), m_dbgInfo( debugInfo ) {}

    const CallId& callId() const {

        DBG_MAIN_THREAD

        return m_callRequest.m_callId;
    }

    const CallRequest& callRequest() const {

        DBG_MAIN_THREAD

        return m_callRequest;
    }

    const CallExecutionResult& callExecutionResult() const {

        DBG_MAIN_THREAD

        return *m_callExecutionResult;
    }

    void setCallExecutionResult( const CallExecutionResult& callExecutionResult ) {

        DBG_MAIN_THREAD

        m_callExecutionResult = callExecutionResult;
    }

    void terminate() {

        DBG_MAIN_THREAD

        if ( m_terminated ) {
            return;
        }

        m_terminated = true;

        if ( m_asyncQuery ) {
            m_asyncQuery->terminate();
        }
    }

    // region internet

    void
    openConnection( const std::string& url,
                    std::function<void( std::optional<uint64_t>&& )>&& callback,
                    std::function<void()>&& terminateCallback ) override {

        DBG_MAIN_THREAD

        _ASSERT( !m_terminated )
        _ASSERT( !m_asyncQuery )

        auto urlDescription = parseURL( url );

        if ( !urlDescription ) {
            _LOG_WARN( "Invalid URL: " << url );
            callback( {} );
            return;
        }

        std::unique_ptr<internet::InternetConnection> connection;

        if ( urlDescription->ssl ) {
            connection = std::make_unique<internet::HttpsInternetConnection>(
                                                                    m_executorEnvironment.sslContext(),
                                                                    m_executorEnvironment.threadManager(),
                                                                    urlDescription->host,
                                                                    urlDescription->port,
                                                                    urlDescription->target,
                                                                    m_executorEnvironment.executorConfig().internetBufferSize(),
                                                                    m_executorEnvironment.executorConfig().internetConnectionTimeoutMilliseconds(),
                                                                    m_executorEnvironment.executorConfig().ocspQueryTimerMilliseconds(),
                                                                    m_executorEnvironment.executorConfig().ocspQueryMaxEfforts(),
                                                                    internet::RevocationVerificationMode::SOFT,
                                                                    m_dbgInfo );
        }
        else {
            connection = std::make_unique<internet::HttpInternetConnection>( m_executorEnvironment.threadManager(),
                                                                   urlDescription->host,
                                                                   urlDescription->port,
                                                                   urlDescription->target,
                                                                   m_executorEnvironment.executorConfig().internetBufferSize(),
                                                                   m_executorEnvironment.executorConfig().internetConnectionTimeoutMilliseconds(),
                                                                   m_dbgInfo );
        }

        auto[connectionIt, insertSuccess] = m_internetConnections.insert(
                {totalConnectionsCreated, std::move( connection )} );

        _ASSERT( insertSuccess )
        auto query = std::make_shared<AbstractAsyncQuery<bool>>( [this, callback = std::move(
                callback ), connectionId = totalConnectionsCreated]
                                                                         ( bool&& success ) {
            // If the callback is executed, 'this' will always be alive
            if ( !success ) {
                auto connectionIt = m_internetConnections.find( connectionId );
                _ASSERT( connectionIt != m_internetConnections.end() )
                connectionIt->second->close();
                m_internetConnections.erase( connectionIt );
                callback( {} );
            } else {
                callback( connectionId );
            }
            m_asyncQuery.reset();
        }, std::move( terminateCallback ), m_executorEnvironment.threadManager());
        totalConnectionsCreated++;
        m_asyncQuery = query;
        connectionIt->second->open( query );
    }

    void read( uint64_t connectionId, std::function<void( std::optional<std::vector<uint8_t>>&& )>&& callback,
               std::function<void()>&& terminateCallback ) override {

        DBG_MAIN_THREAD

        _ASSERT( !m_terminated )
        _ASSERT( !m_asyncQuery )

        auto connectionIt = m_internetConnections.find( connectionId );
        if ( connectionIt == m_internetConnections.end() ) {
            callback( {} );
        }

        auto query = std::make_shared<AbstractAsyncQuery<std::optional<std::vector<uint8_t>>>>(
                [this, callback = std::move(
                        callback ), connectionId = totalConnectionsCreated]
                        ( std::optional<std::vector<uint8_t>>&& data ) {
                    // If the callback is executed, 'this' will always be alive
                    callback( std::move( data ));
                    m_asyncQuery.reset();
                },
                std::move(
                        terminateCallback ),
                m_executorEnvironment.threadManager());
        m_asyncQuery = query;
        connectionIt->second->read( query );
    }

    void closeConnection( uint64_t connectionId, std::function<void( bool )>&& callback,
                std::function<void()>&& terminateCallback ) override {

        DBG_MAIN_THREAD

        _ASSERT( !m_terminated )
        _ASSERT( !m_asyncQuery )

        auto connectionIt = m_internetConnections.find( connectionId );
        if ( connectionIt == m_internetConnections.end() ) {
            callback( false );
        }

        connectionIt->second->close();
        m_internetConnections.erase( connectionIt );
        callback( true );
    }

    // endregion

    // region blockchain

    void getCaller( std::function<void( CallerKey )>&& callback, std::function<void()>&& terminateCallback ) override {

        DBG_MAIN_THREAD

        _ASSERT( m_callRequest.m_callLevel == CallRequest::CallLevel::AUTOMATIC ||
                 m_callRequest.m_callLevel == CallRequest::CallLevel::MANUAL )

        _ASSERT( m_callRequest.m_referenceInfo.m_callerKey )

        CallerKey callerKey = *m_callRequest.m_referenceInfo.m_callerKey;
        callback( std::move( callerKey ) );
    }

    void
    getBlockHeight( std::function<void( uint64_t )>&& callback, std::function<void()>&& terminateCallback ) override {

        DBG_MAIN_THREAD

        callback(m_callRequest.m_referenceInfo.m_blockHeight);
    }

    void
    getBlockHash( std::function<void( BlockHash)>&& callback, std::function<void()>&& terminateCallback ) override {

        DBG_MAIN_THREAD

        callback(m_callRequest.m_referenceInfo.m_blockHash);
    }

    void
    getBlockTime( std::function<void( uint64_t )>&& callback, std::function<void()>&& terminateCallback ) override {

        DBG_MAIN_THREAD

        callback(m_callRequest.m_referenceInfo.m_blockTime);
    }

    void getBlockGenerationTime( std::function<void( uint64_t )>&& callback,
                                 std::function<void()>&& terminateCallback ) override {

        DBG_MAIN_THREAD

        callback(m_callRequest.m_referenceInfo.m_blockGenerationTime);
    }

    void getTransactionHash( std::function<void( TransactionHash )>&& callback,
                             std::function<void()>&& terminateCallback ) override {

        DBG_MAIN_THREAD

        _ASSERT( m_callRequest.m_callLevel == CallRequest::CallLevel::MANUAL )
        _ASSERT( m_callRequest.m_referenceInfo.m_transactionHash )

        callback( *m_callRequest.m_referenceInfo.m_transactionHash );
    }

    // endregion

};

}