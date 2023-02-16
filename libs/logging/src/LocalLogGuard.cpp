/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <logging/LocalLogGuard.h>
#include <mutex>
#include <spdlog/details/thread_pool.h>
#include <iostream>

namespace sirius::logging {

LocalLogGuard::LocalLogGuard(size_t queueSize, size_t numThreads)
        : numThreads(numThreads) {
    m_pPool = std::make_shared<spdlog::details::thread_pool>(queueSize,
                                                             numThreads,
                                                             [] {},
                                                             [this] {
        m_finishedThreadsCounter++;
        m_finishCV.notify_all();
    });
}

std::shared_ptr<spdlog::details::thread_pool> LocalLogGuard::pool() {
    return m_pPool;
}

LocalLogGuard::~LocalLogGuard() {
    m_pPool.reset();
    std::mutex m;
    std::unique_lock lock(m);
    m_finishCV.wait(lock, [this] {
        return m_finishedThreadsCounter == numThreads;
    });
}

}