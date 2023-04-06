/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/
#pragma once

#include <thread>

namespace sirius::contract {

class SingleThread {

protected:

    std::thread::id m_threadId;

protected:

    SingleThread();

    bool isSingleThread() const;

	void setThreadId(const std::thread::id& threadId);

};

}
