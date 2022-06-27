/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <memory>
#include "types.h"

#include "ThreadManager.h"

namespace sirius::contract {

class AsyncQuery {

public:

    virtual ~AsyncQuery() = default;

    virtual void terminate() = 0;

};

// TODO If the query is executed on the Main Thread
// There is no need to use mutex
template <class TReply>
class AbstractAsyncQuery:
        public AsyncQuery,
        public std::enable_shared_from_this<AbstractAsyncQuery<TReply>> {

private:

    enum class Status {
        ACTIVE,
        EXECUTED,
        TERMINATED
    };

    ThreadManager& m_threadManager;

    Status m_status = Status::ACTIVE;
    std::mutex m_statusMutex;

    std::function<void( TReply&& )> m_callback;
    std::function<void()> m_terminateCallback;

public:

    AbstractAsyncQuery( std::function<void( TReply&& )> callback,
                        std::function<void()> terminateCallback,
                        ThreadManager& threadManager )
                        : m_callback( std::move(callback) )
                        , m_terminateCallback( std::move(terminateCallback) )
                        , m_threadManager( threadManager )
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

    void postReply( TReply&& reply ) {
        // NOT MAIN THREAD
        std::lock_guard<std::mutex> lock( m_statusMutex);
        if ( m_status != Status::ACTIVE ) {
            return;
        }
        // If is not terminated, then it is guaranteed that ThreadManager is valid
        m_threadManager.execute([pWeakThis = this->weak_from_this(), reply=std::move(reply)] () mutable {
            // MAIN THREAD
            if ( auto pThis = pWeakThis.lock(); pThis ) {
                // No mutex is required since the value of m_status can be changed only on the main thread
                if ( pThis->m_status != Status::ACTIVE ) {
                    return;
                }
                pThis->m_status = Status::EXECUTED;
                pThis->m_callback( std::move(reply) );
            }
        });
    }
};

}