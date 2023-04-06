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

private:

    boost::asio::io_context m_context;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_work;
    std::thread m_thread;

public:
    ThreadManager();

    ~ThreadManager();

    void stop();

public:

    template<class Function>
    void execute( Function&& task ) {
        boost::asio::post( m_context, std::forward<Function>(task));
    }

    template<class Function>
    Timer
    startTimer(int milliseconds, Function&& func) {
        return {m_context, milliseconds, std::forward<Function>(func)};;
    }

    boost::asio::io_context& context();

    std::thread::id threadId();
};

}
