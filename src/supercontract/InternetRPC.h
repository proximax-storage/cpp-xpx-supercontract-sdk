/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "internet.grpc.pb.h"
#include "RPCInternetRequests.h"

namespace sirius::contract {

    class RPCService {

    public:

        virtual ~RPCService() = default;

        virtual void registerService(grpc::ServerBuilder&) = 0;

        virtual void runService() = 0;

    };

    template <class TService>
    class RPCServiceImpl : public RPCService {

    protected:

        TService m_service;
        std::unique_ptr<grpc::ServerCompletionQueue> m_completionQueue;

    private:

        std::thread m_completionQueueThread;

    public:

        RPCServiceImpl() = default;

        ~RPCServiceImpl() override {
            if ( m_completionQueue ) {
                m_completionQueue->Shutdown();
            }
            if ( m_completionQueueThread.joinable() ) {
                m_completionQueueThread.join();
            }
        }

    public:

        void registerService( grpc::ServerBuilder& builder ) override {
            builder.RegisterService(&m_service);
            m_completionQueue = builder.AddCompletionQueue();
        }

        void runService() override {
            m_completionQueueThread = [this] {
                registerCalls();
                handleCalls();
            };
        }

    private:

        virtual void registerCalls() = 0;

        virtual void handleCalls() {
            void* tag;
            bool ok;
            while (m_completionQueue->Next(&tag, &ok)) {
                auto* call = static_cast<RPCCall *>(tag);
                if (ok) {
                    call->process();
                }
                else {
                    delete call;
                }
            }
        }
    };

    class InternetService: public RPCServiceImpl<internet::Internet::AsyncService> {
        void registerCalls() override {
            new OpenConnectionRPCInternetRequest(&m_service, m_completionQueue.get());
        }
    };
}