/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "grpc++/server_builder.h"
#include "RPCInternetRequests.h"

namespace sirius::contract {

    class RPCServerServiceManager {

    private:
        const SessionId m_sessionId;
        ThreadManager& m_threadManager;

        std::vector<std::unique_ptr<RPCService>> m_services;

        std::unique_ptr<grpc::ServerBuilder> m_builder;
        std::unique_ptr<grpc::Server> m_server;

    public:

        RPCServerServiceManager( const std::string& serverAddress, const SessionId& sessionId, ThreadManager& threadManager )
        : m_sessionId(sessionId)
        , m_threadManager( threadManager ) {
            m_builder = std::make_unique<grpc::ServerBuilder>();
            m_builder->AddListeningPort( serverAddress, grpc::InsecureServerCredentials());
        }

        template<class TService, class THandler>
        std::weak_ptr<VirtualMachineQueryHandlersKeeper<THandler>> addService() {
            auto handler = std::make_shared<VirtualMachineQueryHandlersKeeper<THandler>>();
            auto service = std::make_unique<TService>( m_sessionId, handler, m_threadManager );
            service->registerService( *m_builder );
            m_services.push_back( std::move( service ));
            return handler;
        }

        void run() {
            m_server = m_builder->BuildAndStart();
            m_builder.reset();
            for (auto& service: m_services) {
                service->runService();
            }
        }

        void terminate() {
            m_server->Shutdown();
            for (auto& service: m_services) {
                service->terminate();
            }
        }

    };

}