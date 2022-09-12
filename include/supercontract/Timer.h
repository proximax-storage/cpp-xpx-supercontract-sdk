/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#pragma once

#include <memory>
#include <boost/asio/high_resolution_timer.hpp>

namespace sirius::contract {

class Timer {

private:

    class TimerCallbackInterface {

    public:

        virtual ~TimerCallbackInterface() = default;

        virtual void run(int milliseconds) = 0;

    };

    template<class Function>
    class TimerCallback
            : public TimerCallbackInterface, public std::enable_shared_from_this<TimerCallback<Function>> {

    private:

        boost::asio::high_resolution_timer m_timer;
        Function m_callback;

    public:

        template<class ExecutionContext>
        explicit TimerCallback(
                ExecutionContext& executionContext,
                Function&& callback)
                : m_timer(executionContext)
                , m_callback(callback) {}

        void run(int milliseconds) override {
            m_timer.expires_after(std::chrono::milliseconds(milliseconds));
            m_timer.async_wait([pThisWeak = TimerCallback::weak_from_this()](boost::system::error_code const& ec) {
                if (auto pThis = pThisWeak.lock(); pThis) {
                    pThis->onTimeout(ec);
                }
            });
        }

    private:

        void onTimeout(const boost::system::error_code& ec) {
            if (ec) {
                return;
            }

            m_callback();
        }

    };

    std::shared_ptr<TimerCallbackInterface> m_callback;

public:

    Timer(const Timer&) = delete;

    Timer& operator=(const Timer&) = delete;

    Timer(Timer&&) noexcept = default;

    Timer& operator=(Timer&&) noexcept = default;

    template<class ExecutionContext, class Function>
    Timer(ExecutionContext& executionContext,
          int milliseconds,
          Function&& callback)
            : m_callback(
            std::make_shared<TimerCallback<Function>>(executionContext, std::forward<Function>(callback))) {
        m_callback->run(milliseconds);
    }

    Timer() = default;

    operator bool() const {
        return m_callback.operator bool();
    }

    void cancel() {
        m_callback.reset();
    }
};

}