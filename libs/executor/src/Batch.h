/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <deque>
#include <list>
#include <vector>

#include "supercontract/Requests.h"
#include "virtualMachine/CallRequest.h"

namespace sirius::contract {

struct Batch {
    uint64_t m_batchIndex;
    std::deque<vm::CallRequest> m_callRequests;
};

}