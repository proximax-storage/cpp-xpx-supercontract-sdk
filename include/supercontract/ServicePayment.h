/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "Identifiers.h"

namespace sirius::contract {

struct ServicePayment {
    uint64_t m_mosaicId;
    uint64_t m_amount;
};

}