/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <crypto/CurvePoint.h>
#include "Proofs.h"

namespace sirius::contract {

struct ExecutorInfo {
	uint64_t NextBatchToApprove = 0;
	Proofs PoEx;
};

}