/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <common/ThreadManager.h>

namespace sirius::contract {

ThreadManager::ThreadManager(): m_work(boost::asio::make_work_guard(m_context))
                                , m_thread(std::thread([this] { m_context.run(); })) {}

ThreadManager::~ThreadManager() {
    stop();
}

void ThreadManager::stop() {
    m_work.reset();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

boost::asio::io_context& ThreadManager::context() {
    return m_context;
}

std::thread::id ThreadManager::threadId() {
    return m_thread.get_id();
}

}