/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#pragma once

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <thread>

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

    std::optional<boost::asio::high_resolution_timer>
    startTimer( int milliseconds, const std::function<void()>& func ) {
        boost::asio::high_resolution_timer timer( m_context );

        timer.expires_after( std::chrono::milliseconds( milliseconds ));
        timer.async_wait( [func = func]( boost::system::error_code const& e ) {
            if ( !e ) {
                func();
            }
        } );

        return timer;
    }

    boost::asio::io_context& context() {
        return m_context;
    }

    auto threadId() {
        return m_thread.get_id();
    }
};

}
