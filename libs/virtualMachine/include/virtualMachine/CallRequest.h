/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <cstdint>
#include <supercontract/CallRequest.h>

namespace sirius::contract::vm {

struct CallRequest : CallRequestParameters {

    enum class CallLevel {
        AUTORUN,
        AUTOMATIC,
        MANUAL
    };

    CallLevel m_callLevel;
    uint64_t m_proofOfExecutionPrefix = 0;

    CallRequest(const CallRequestParameters& parameters, const CallLevel& callLevel)
            : CallRequestParameters(parameters)
            , m_callLevel(callLevel) {}
};

struct CallExecutionResult {
    bool m_success;
    uint32_t m_return;
    uint64_t m_scConsumed;
    uint64_t m_smConsumed;
    uint64_t m_proofOfExecutionSecretData;
};

}