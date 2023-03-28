/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <crypto/CurvePoint.h>
#include <blockchain/Proofs.h>

namespace sirius::contract {

struct ExecutorInfo {
	uint64_t m_nextBatchToApprove = 0;
	uint64_t m_initialBatch = 0;
	blockchain::BatchProof m_batchProof;
};

}