/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#pragma once

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <thread>

#include "Timer.h"

namespace sirius::contract {

class ThreadManager {
    boost::asio::io_context m_context;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_work;
    std::thread m_thread;

public:
    ThreadManager()
            : m_context()
            , m_work( boost::asio::make_work_guard( m_context ))
            , m_thread( std::thread( [this] { m_context.run(); } )) {
    }

    ~ThreadManager() {
        stop();
    }

    void stop() {
        m_work.reset();
        if ( m_thread.joinable() ) {
            m_thread.join();
        }
    }

    template<class Function>
    void execute( Function&& task ) {
        boost::asio::post( m_context, std::forward<Function>(task));
    }

    template<class Function>
    Timer
    startTimer(int milliseconds, Function&& func) {
        return {m_context, milliseconds, std::forward<Function>(func)};;
    }

    boost::asio::io_context& context() {
        return m_context;
    }

    auto threadId() {
        return m_thread.get_id();
    }
};

}
