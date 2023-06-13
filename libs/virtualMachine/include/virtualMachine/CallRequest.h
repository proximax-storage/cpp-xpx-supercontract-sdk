/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <cstdint>
#include <blockchain/AggregatedTransaction.h>
#include "CallRequest.h"

namespace sirius::contract::vm {

struct CallRequest {

    enum class CallLevel {
        AUTORUN,
        AUTOMATIC,
        MANUAL
    };

    CallId m_callId;
    std::string m_file;
    std::string m_function;
    std::vector<uint8_t> m_params;
    uint64_t m_executionGasLimit;
    uint64_t m_downloadGasLimit;
    CallLevel m_callLevel;
    uint64_t m_proofOfExecutionPrefix;
	DriveKey m_driveKey;

    CallRequest(const CallId& callId,
                const std::string& file,
                const std::string& function,
                const std::vector<uint8_t>& params,
                uint64_t executionGasLimit,
                uint64_t downloadGasLimit,
                CallLevel callLevel,
                uint64_t proofOfExecutionPrefix,
				const DriveKey& driveKey)
                : m_callId(callId)
                , m_file(file)
                , m_function(function)
                , m_params(params)
                , m_executionGasLimit(executionGasLimit)
                , m_downloadGasLimit(downloadGasLimit)
                , m_callLevel(callLevel)
                , m_proofOfExecutionPrefix(proofOfExecutionPrefix)
				, m_driveKey(driveKey) {}
};

struct CallExecutionResult {
    bool m_success = false;
    uint32_t m_return = 0;
    uint64_t m_execution_gas_consumed = 0;
    uint64_t m_download_gas_consumed = 0;
    uint64_t m_proofOfExecutionSecretData = 0;
    std::optional<blockchain::AggregatedTransaction> m_transaction;
};

}