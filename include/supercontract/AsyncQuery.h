/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <memory>
#include "Identifiers.h"

#include "GlobalEnvironment.h"

namespace sirius::contract {

class AsyncQuery {

public:

    virtual ~AsyncQuery() = default;

    virtual void terminate() = 0;
};

template<class TReply>
class AsyncCallback {

public:

    virtual void postReply( TReply&& ) = 0;

    virtual bool isTerminated() const = 0;
};

// TODO If the query is executed on the Main Thread
// There is no need to use mutex
template <class TReply, class TCallback, class TTerminateCallback>
class AsyncQueryHandler:
        public AsyncQuery,
        public AsyncCallback<TReply>,
        public std::enable_shared_from_this<AsyncQueryHandler<TReply, TCallback, TTerminateCallback>> {

private:

    enum class Status {
        ACTIVE,
        EXECUTED,
        TERMINATED
    };

    GlobalEnvironment& m_globalEnvironment;

    Status m_status = Status::ACTIVE;
    std::mutex m_statusMutex;

    TCallback          m_callback;
    TTerminateCallback m_terminateCallback;

public:

    AsyncQueryHandler( TCallback&& callback,
                       TTerminateCallback&& terminateCallback,
                       GlobalEnvironment& globalEnvironment )
                       : m_callback( std::move(callback) )
                       , m_terminateCallback( std::move(terminateCallback) )
                       , m_globalEnvironment( globalEnvironment )
                       {}

    void terminate() override {
        // MAIN THREAD
        std::lock_guard<std::mutex> lock( m_statusMutex);
        if ( m_status != Status::ACTIVE ) {
            return;
        }
        m_status = Status::TERMINATED;
        m_terminateCallback();
    }

    void postReply( TReply&& reply ) override {
        // NOT MAIN THREAD
        std::lock_guard<std::mutex> lock( m_statusMutex);
        if ( m_status != Status::ACTIVE ) {
            return;
        }

        // If is not terminated, then it is guaranteed that ThreadManager is valid
        m_globalEnvironment.threadManager().execute([pThis = this->shared_from_this(), reply=std::move(reply)] () mutable {
            // MAIN THREAD
            // No mutex is required since the value of m_status can be changed only on the main thread
            if ( pThis->m_status != Status::ACTIVE ) {
                return;
            }
            pThis->m_status = Status::EXECUTED;
            pThis->m_callback( std::move(reply) );
        });
    }

public:

    bool isTerminated() const override {
        return m_status == Status::TERMINATED;
    }
};

template <class TReply, class TCallback, class TTerminateCallback>
std::shared_ptr<AsyncQueryHandler<TReply, TCallback, TTerminateCallback>> createAsyncQueryHandler(TCallback&& callback,
                                                                                                  TTerminateCallback&& terminateCallback,
                                                                                                  GlobalEnvironment& env) {
    return std::make_shared<AsyncQueryHandler<TReply, TCallback, TTerminateCallback>>(std::forward<TCallback>(callback), std::forward<TTerminateCallback>(terminateCallback), env);
}

}