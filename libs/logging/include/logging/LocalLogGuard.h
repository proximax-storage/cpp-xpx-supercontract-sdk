/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <spdlog/async_logger.h>
#include <condition_variable>

namespace sirius::logging {

class LocalLogGuard {

private:

    const uint numThreads;
    std::shared_ptr<spdlog::details::thread_pool> m_pPool;

    std::condition_variable m_finishCV;
    std::atomic_uint m_finishedThreadsCounter = 0;

public:

    LocalLogGuard(size_t queueSize, size_t numThreads);

    std::shared_ptr<spdlog::details::thread_pool> pool();

    ~LocalLogGuard();

};

}