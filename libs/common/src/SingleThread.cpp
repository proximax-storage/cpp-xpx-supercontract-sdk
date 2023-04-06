/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <common/SingleThread.h>

namespace sirius::contract {

SingleThread::SingleThread(): m_threadId(std::this_thread::get_id()) {}

bool SingleThread::isSingleThread() const {
    return m_threadId == std::this_thread::get_id();
}

void SingleThread::setThreadId(const std::thread::id& threadId) {
    m_threadId = threadId;
}

}
