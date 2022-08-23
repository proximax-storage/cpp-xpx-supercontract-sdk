/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#pragma once

#include <thread>

namespace sirius::contract {

//#define DBG_MAIN_THREAD // { _FUNC_ENTRY(); ASSERT( m_threadId == std::this_thread::get_id() ); }

class SingleThread {

protected:

    std::thread::id m_threadId;

    SingleThread(): m_threadId(std::this_thread::get_id()) {}

    bool isSingleThread() {
        return m_threadId == std::this_thread::get_id();
    }

};

}
