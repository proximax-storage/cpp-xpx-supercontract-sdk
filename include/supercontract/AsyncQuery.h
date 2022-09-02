/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <memory>
#include "Identifiers.h"

#include "GlobalEnvironment.h"
#include "SingleThread.h"

namespace sirius::contract {

class AsyncQuery {

public:

    virtual ~AsyncQuery() = default;

    virtual void terminate() = 0;
};

template<class TReply>
class AsyncQueryCallback {

public:

    virtual void postReply(TReply&&) = 0;

    virtual bool isTerminated() const = 0;
};

// TODO If the query is executed on the Main Thread
// There is no need to use mutex
template<class TReply, class TCallback, class TTerminateCallback>
class AsyncCallbackAsyncQuery
        :
                private SingleThread,
                public AsyncQuery,
                public AsyncQueryCallback<TReply>,
                public std::enable_shared_from_this<AsyncCallbackAsyncQuery<TReply, TCallback, TTerminateCallback>> {

private:

    enum class Status {
        ACTIVE,
        EXECUTED,
        TERMINATED
    };

    GlobalEnvironment& m_globalEnvironment;

    Status m_status = Status::ACTIVE;
    std::mutex m_statusMutex;

    TCallback m_callback;
    TTerminateCallback m_terminateCallback;

public:

    AsyncCallbackAsyncQuery(TCallback&& callback,
                            TTerminateCallback&& terminateCallback,
                            GlobalEnvironment& globalEnvironment)
            : m_callback(std::move(callback))
            , m_terminateCallback(std::move(terminateCallback))
            , m_globalEnvironment(globalEnvironment) {}

    void terminate() override {

        ASSERT(isSingleThread(), m_globalEnvironment.logger())

        std::lock_guard<std::mutex> lock(m_statusMutex);
        if (m_status != Status::ACTIVE) {
            return;
        }
        m_status = Status::TERMINATED;
        m_terminateCallback();
    }

    void postReply(TReply&& reply) override {
        // Any thread is possible
        std::lock_guard<std::mutex> lock(m_statusMutex);
        if (m_status != Status::ACTIVE) {
            return;
        }

        // If is not terminated, then it is guaranteed that ThreadManager is valid
        m_globalEnvironment.threadManager().execute(
                [pThis = this->shared_from_this(), reply = std::move(reply)]() mutable {

                    ASSERT(pThis->isSingleThread(), pThis->m_globalEnvironment.logger())

                    // No mutex is required since the value of m_status can be changed only on the main thread
                    if (pThis->m_status != Status::ACTIVE) {
                        return;
                    }
                    pThis->m_status = Status::EXECUTED;
                    pThis->m_callback(std::move(reply));
                });
    }

public:

    bool isTerminated() const override {
        return m_status == Status::TERMINATED;
    }
};

template<class TReply, class TCallback, class TTerminateCallback>
std::shared_ptr<AsyncCallbackAsyncQuery<TReply, TCallback, TTerminateCallback>>
createAsyncCallbackAsyncQuery(TCallback&& callback,
                              TTerminateCallback&& terminateCallback,
                              GlobalEnvironment& env) {
    return std::make_shared<AsyncCallbackAsyncQuery<TReply, TCallback, TTerminateCallback>>(
            std::forward<TCallback>(callback), std::forward<TTerminateCallback>(terminateCallback), env);
}

template<class TReply, class TCallback, class TTerminateCallback>
class SyncCallbackAsyncQuery
        :
                private SingleThread,
                public AsyncQuery,
                public AsyncQueryCallback<TReply>,
                public std::enable_shared_from_this<AsyncCallbackAsyncQuery<TReply, TCallback, TTerminateCallback>> {

private:

    enum class Status {
        ACTIVE,
        EXECUTED,
        TERMINATED
    };

    GlobalEnvironment& m_globalEnvironment;

    Status m_status = Status::ACTIVE;

    TCallback m_callback;
    TTerminateCallback m_terminateCallback;

public:

    SyncCallbackAsyncQuery(TCallback&& callback,
                           TTerminateCallback&& terminateCallback,
                           GlobalEnvironment& globalEnvironment)
            : m_callback(std::move(callback))
            , m_terminateCallback(std::move(terminateCallback))
            , m_globalEnvironment(globalEnvironment) {}

    void terminate() override {

        ASSERT(isSingleThread(), m_globalEnvironment.logger())

        if (m_status != Status::ACTIVE) {
            return;
        }
        m_status = Status::TERMINATED;
        m_terminateCallback();
    }

    void postReply(TReply&& reply) override {

        ASSERT(isSingleThread(), m_globalEnvironment.logger())

        if (m_status != Status::ACTIVE) {
            return;
        }

        m_status = Status::EXECUTED;
        m_callback(std::move(reply));
    }

public:

    bool isTerminated() const override {
        return m_status == Status::TERMINATED;
    }
};

template<class TReply, class TCallback, class TTerminateCallback>
std::shared_ptr<AsyncCallbackAsyncQuery<TReply, TCallback, TTerminateCallback>>
createSyncCallbackAsyncQuery(TCallback&& callback,
                             TTerminateCallback&& terminateCallback,
                             GlobalEnvironment& env) {
    return std::make_shared<SyncCallbackAsyncQuery<TReply, TCallback, TTerminateCallback>>(
            std::forward<TCallback>(callback), std::forward<TTerminateCallback>(terminateCallback), env);
}

}