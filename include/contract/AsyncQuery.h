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

template <class TReply>
class AbstractAsyncQuery:
        public AsyncQuery,
        public std::enable_shared_from_this<AbstractAsyncQuery<TReply>> {

private:

    ThreadManager& m_threadManager;

    bool m_terminated = false;
    std::mutex m_terminateMutex;

    std::function<void( TReply&& )> m_callback;

public:

    AbstractAsyncQuery( std::function<void( TReply&& )> callback,
                        ThreadManager& threadManager )
                        : m_callback(std::move(callback))
                        , m_threadManager(threadManager)
                        {}

    void terminate() override {
        // MAIN THREAD
        std::lock_guard<std::mutex> lock(m_terminateMutex);
        m_terminated = true;
    }

    void postReply( TReply&& reply ) {
        // NOT MAIN THREAD
        std::lock_guard<std::mutex> lock(m_terminateMutex);
        if ( m_terminated ) {
            return;
        }
        // If is not terminated, then it is guaranteed that ThreadManager is valid
        m_threadManager.execute([pWeakThis = this->weak_from_this(), reply=std::move(reply)] () mutable {
            // MAIN THREAD
            if ( auto pThis = pWeakThis.lock(); pThis ) {
                // No mutex is required since the value of m_terminated can be changed only on the main thread
                if ( pThis->m_terminated ) {
                    return;
                }
                pThis->m_callback( std::move(reply) );
            }
        });
    }
};

}