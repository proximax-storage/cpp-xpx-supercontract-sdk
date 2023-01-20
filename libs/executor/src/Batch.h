/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <deque>
#include <list>
#include <vector>
#include <memory>

#include <executor/Requests.h>
#include <executor/CallRequest.h>

namespace sirius::contract {


struct Batch {
    uint64_t m_batchIndex;
    uint64_t m_automaticExecutionsCheckedUpTo;
    std::deque<std::shared_ptr<CallRequest>> m_callRequests;
};

}