/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "types.h"
#include "contract/AsyncQuery.h"
#include "HttpInternetConnection.h"
#include "HttpsInternetConnection.h"

namespace sirius::contract {

class CallExecutionEnvironment : public VirtualMachineInternetQueryHandler {

private:

    ExecutorEnvironment& m_executorEnvironment;
    ContractEnvironment& m_contractEnvironment;

    const CallRequest m_callRequest;

    std::optional<CallExecutionResult> m_callExecutionResult;

    std::shared_ptr<AsyncQuery> m_asyncQuery;

    uint64_t totalConnectionsCreated = 0;
    std::map<uint64_t, std::unique_ptr<InternetConnection>> m_internetConnections;

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

        if ( m_asyncQuery ) {
            m_asyncQuery->terminate();
        }
    }

    // region internet

    void
    openConnection( const std::string& host,
                    const std::string& target,
                    std::function<void( std::optional<uint64_t>&& )>&& callback,
                    std::function<void()>&& terminateCallback ) override {

        DBG_MAIN_THREAD

        _ASSERT( !m_asyncQuery )

        auto[connectionIt, insertSuccess] = m_internetConnections.insert(
                {totalConnectionsCreated,
                 std::make_unique<HttpInternetConnection>( m_executorEnvironment.threadManager(), host, target,
                                                              m_executorEnvironment.executorConfig().internetBufferSize(),
                                                              m_executorEnvironment.executorConfig().internetConnectionTimeoutMilliseconds(),
                                                              m_dbgInfo )} );
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

    void close( uint64_t connectionId, std::function<void( bool )>&& callback,
                std::function<void()>&& terminateCallback ) override {

        DBG_MAIN_THREAD

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

};

}