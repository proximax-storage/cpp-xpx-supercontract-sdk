/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "Identifiers.h"

namespace sirius::contract {

struct CallReferenceInfo {
    std::optional<CallerKey> m_callerKey;
    uint64_t m_blockHeight;
};

struct CallRequestParameters {
    CallId m_callId;
    std::string m_file;
    std::string m_function;
    std::vector<uint8_t> m_params;
    uint64_t m_scLimit;
    uint64_t m_smLimit;
    CallReferenceInfo m_referenceInfo;
};

}