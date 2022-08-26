/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <grpcpp/server_builder.h>
#include <supercontract/Identifiers.h>
#include "supercontract/GlobalEnvironment.h"
#include "supercontract/SingleThread.h"

#include "virtualMachine/VirtualMachineQueryHandlersKeeper.h"
#include "RPCCall.h"

namespace sirius::contract::vm {

    class RPCService {

    public:

        virtual ~RPCService() = default;

        virtual void registerService(grpc::ServerBuilder&) = 0;

        virtual void runService() = 0;

        virtual void terminate() = 0;

    };

    template <class TService, class THandler>
    class RPCServiceImpl :
            protected SingleThread,
            public RPCService {

    protected:

        const SessionId                              m_sessionId;
        TService                                     m_service;
        std::unique_ptr<grpc::ServerCompletionQueue> m_completionQueue;
        std::shared_ptr<VirtualMachineQueryHandlersKeeper<THandler>> m_handlersExtractor;
        bool                                         m_terminated = false;

        GlobalEnvironment&  m_environment;

    private:
        std::thread         m_completionQueueThread;

    public:

        explicit RPCServiceImpl( const SessionId& sessionId,
                                 const std::shared_ptr<VirtualMachineQueryHandlersKeeper<THandler>>& handlersExtractor,
                                 GlobalEnvironment& environment )
        : m_sessionId( sessionId )
        , m_handlersExtractor( handlersExtractor )
        , m_environment( environment )
        {}

        void terminate() override {

            ASSERT(isSingleThread(), m_environment.logger())

            m_terminated = true;
            m_handlersExtractor.template reset();
            if ( m_completionQueue ) {
                m_completionQueue->Shutdown();
            }
            if ( m_completionQueueThread.joinable() ) {
                m_completionQueueThread.join();
            }
        }

    public:

        void registerService( grpc::ServerBuilder& builder ) override {

            ASSERT(isSingleThread(), m_environment.logger())

            builder.RegisterService(&m_service);
            m_completionQueue = builder.AddCompletionQueue();
        }

        void runService() override {

            ASSERT(isSingleThread(), m_environment.logger())

            registerCalls();

            m_completionQueueThread = std::thread( [this] {
                handleCalls();
            } );
        }

    private:

        virtual void registerCalls() = 0;

        virtual void handleCalls() {

            ASSERT(!isSingleThread(), m_environment.logger())

            void* tag;
            bool ok;
            while (m_completionQueue->Next(&tag, &ok)) {
                auto* call = static_cast<RPCCall *>(tag);
                if (ok) {
                    m_environment.threadManager().execute( [call] {
                        call->process();
                    } );
                }
                else {
                    delete call;
                }
            }
        }
    };
}