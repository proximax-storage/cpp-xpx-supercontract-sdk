/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <cstdint>

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

    CallRequest(const CallId& callId,
                const std::string& file,
                const std::string& function,
                const std::vector<uint8_t>& params,
                uint64_t executionGasLimit,
                uint64_t downloadGasLimit,
                CallLevel callLevel,
                uint64_t proofOfExecutionPrefix)
                : m_callId(callId)
                , m_file(file)
                , m_function(function)
                , m_params(params)
                , m_executionGasLimit(executionGasLimit)
                , m_downloadGasLimit(downloadGasLimit)
                , m_callLevel(callLevel)
                , m_proofOfExecutionPrefix(proofOfExecutionPrefix) {}
};

struct CallExecutionResult {
    bool m_success;
    uint32_t m_return;
    uint64_t m_execution_gas_consumed;
    uint64_t m_download_gas_limit;
    uint64_t m_proofOfExecutionSecretData;
};

}