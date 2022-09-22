/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <memory>
#include <system_error>
#include "Identifiers.h"

#include "GlobalEnvironment.h"
#include "SingleThread.h"
#include <tl/expected.hpp>

namespace sirius::contract {

template<class T>
using expected = tl::expected<T, std::error_code>;

class QueryStatusManager {

public:

    enum class Status {
        ACTIVE,
        EXECUTED,
        TERMINATED
    };

private:

    Status m_status = Status::ACTIVE;

    std::mutex m_statusMutex;

public:

    void setStatus(const Status& status) {
        m_status = status;
    }

    Status status() {
        return m_status;
    }

    std::mutex& statusMutex() {
        return m_statusMutex;
    }
};

class AsyncQuery {

public:

    virtual ~AsyncQuery() = default;

    virtual void terminate() = 0;
};

template<class TTerminateCallback>
class ManualTerminateAsyncQuery
        : public AsyncQuery, private SingleThread {

private:

    GlobalEnvironment& m_globalEnvironment;
    std::shared_ptr<QueryStatusManager> m_statusManager;
    TTerminateCallback m_terminateCallback;

public:

    ManualTerminateAsyncQuery(TTerminateCallback
                              && terminateCallback,
                              std::shared_ptr<QueryStatusManager> statusManager,
                              GlobalEnvironment
                              & globalEnvironment)
            : m_terminateCallback(std::move(terminateCallback))
            , m_statusManager(std::move(statusManager))
            , m_globalEnvironment(globalEnvironment) {}

    void terminate() final {
        ASSERT(isSingleThread(), m_globalEnvironment.logger())

        std::lock_guard<std::mutex> lock(m_statusManager->statusMutex());
        if (m_statusManager->status() != QueryStatusManager::Status::ACTIVE) {
            return;
        }
        m_statusManager->setStatus(QueryStatusManager::Status::TERMINATED);
        m_terminateCallback();
    }
};

template<class TTerminateCallback>
class AutoTerminateAsyncQuery
        : public AsyncQuery, private SingleThread {

private:

    GlobalEnvironment& m_globalEnvironment;
    std::shared_ptr<QueryStatusManager> m_statusManager;
    TTerminateCallback m_terminateCallback;

public:

    AutoTerminateAsyncQuery(TTerminateCallback&& terminateCallback,
                            std::shared_ptr<QueryStatusManager> statusManager,
                            GlobalEnvironment& globalEnvironment)
            : m_terminateCallback(std::move(terminateCallback))
            , m_statusManager(std::move(statusManager))
            , m_globalEnvironment(globalEnvironment) {}

    ~AutoTerminateAsyncQuery() override {
        terminate();
    }

    void terminate() final {
        ASSERT(isSingleThread(), m_globalEnvironment.logger())

        std::lock_guard<std::mutex> lock(m_statusManager->statusMutex());
        if (m_statusManager->status() != QueryStatusManager::Status::ACTIVE) {
            return;
        }
        m_statusManager->setStatus(QueryStatusManager::Status::TERMINATED);
        m_terminateCallback();
    }
};

template<class TReply>
class AsyncQueryCallback {

public:

    virtual void postReply(tl::expected<TReply, std::error_code>&&) = 0;

    virtual bool isTerminated() const = 0;
};

// TODO If the query is executed on the Main Thread
// There is no need to use mutex
template<class TReply, class TCallback>
class AsyncCallbackAsyncQuery
        : private SingleThread,
          public AsyncQueryCallback<TReply>,
          public std::enable_shared_from_this<AsyncCallbackAsyncQuery<TReply, TCallback>> {

private:

    GlobalEnvironment& m_globalEnvironment;
    std::shared_ptr<QueryStatusManager> m_statusManager;
    TCallback m_callback;

public:

    AsyncCallbackAsyncQuery(TCallback&& callback,
                            std::shared_ptr<QueryStatusManager> statusManager,
                            GlobalEnvironment& globalEnvironment)
            : m_callback(std::move(callback))
            , m_statusManager(std::move(statusManager))
            , m_globalEnvironment(globalEnvironment) {}

    void postReply(tl::expected<TReply, std::error_code>&& reply) override {
        // Any thread is possible
        std::lock_guard<std::mutex> lock(m_statusManager->statusMutex());
        if (m_statusManager->status() != QueryStatusManager::Status::ACTIVE) {
            return;
        }

        // If is not terminated, then it is guaranteed that ThreadManager is valid
        m_globalEnvironment.threadManager().execute(
                [pThis = this->shared_from_this(), reply = std::move(reply)]() mutable {

                    ASSERT(pThis->isSingleThread(), pThis->m_globalEnvironment.logger())

                    // No mutex is required since the value of m_status can be changed only on the main thread
                    if (pThis->m_statusManager->status() != QueryStatusManager::Status::ACTIVE) {
                        return;
                    }
                    pThis->m_statusManager->setStatus(QueryStatusManager::Status::EXECUTED);
                    pThis->m_callback(std::move(reply));
                });
    }

public:

    bool isTerminated() const override {
        return m_statusManager->status() == QueryStatusManager::Status::TERMINATED;
    }
};

template<class TReply, class TCallback>
class SyncCallbackAsyncQuery
        : private SingleThread
        , public AsyncQueryCallback<TReply>
        , public std::enable_shared_from_this<SyncCallbackAsyncQuery<TReply, TCallback>> {

private:

    GlobalEnvironment& m_globalEnvironment;
    std::shared_ptr<QueryStatusManager> m_statusManager;
    TCallback m_callback;

public:

    SyncCallbackAsyncQuery(TCallback&& callback,
                           std::shared_ptr<QueryStatusManager> statusManager,
                           GlobalEnvironment& globalEnvironment)
            : m_callback(std::move(callback))
            , m_statusManager(std::move(statusManager))
            , m_globalEnvironment(globalEnvironment) {}

    void postReply(tl::expected<TReply, std::error_code>&& reply) override {

        ASSERT(isSingleThread(), m_globalEnvironment.logger())

        if (m_statusManager->status() != QueryStatusManager::Status::ACTIVE) {
            return;
        }

        m_statusManager->setStatus(QueryStatusManager::Status::EXECUTED);
        m_callback(std::move(reply));
    }

public:

    bool isTerminated() const override {
        return m_statusManager->status() == QueryStatusManager::Status::TERMINATED;
    }
};

template<class TReply, class TCallback, class TTerminateCallback>
std::pair<std::shared_ptr<AsyncQuery>, std::shared_ptr<AsyncQueryCallback<TReply>>>
createAsyncQuery(TCallback&& callback,
                 TTerminateCallback&& terminateCallback,
                 GlobalEnvironment& env,
                 bool terminateQueryOnDestroy,
                 bool asyncCallback) {
    auto statusManager = std::make_shared<QueryStatusManager>();

    std::shared_ptr<AsyncQuery> query;

    if (terminateQueryOnDestroy) {
        query = std::make_shared<AutoTerminateAsyncQuery<TTerminateCallback>>(std::forward<TTerminateCallback>(terminateCallback), statusManager, env);
    } else {
        query = std::make_shared<ManualTerminateAsyncQuery<TTerminateCallback>>(std::forward<TTerminateCallback>(terminateCallback), statusManager, env);
    }

    std::shared_ptr<AsyncQueryCallback<TReply>> back;

    if (asyncCallback) {
        back = std::make_shared<AsyncCallbackAsyncQuery<TReply, TCallback>>(
                std::forward<TCallback>(callback), statusManager, env);
    } else {
        back = std::make_shared<SyncCallbackAsyncQuery<TReply, TCallback>>(
                std::forward<TCallback>(callback), statusManager, env);
    }

    return {query, back};
}

}