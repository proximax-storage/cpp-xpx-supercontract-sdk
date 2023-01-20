/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <executor/CallRequest.h>

namespace sirius::contract {

CallRequest::CallRequest(const CallId& callId,
                         const std::string& file,
                         const std::string& function,
                         uint64_t executionPayment,
                         uint64_t downloadPayment,
                         const CallerKey& callerKey,
                         uint64_t blockHeight)
        : m_callId(callId)
          , m_file(file)
          , m_function(function)
          , m_executionPayment(executionPayment)
          , m_downloadPayment(downloadPayment)
          , m_callerKey(callerKey)
          , m_blockHeight(blockHeight) {}

CallId CallRequest::callId() const {
    return m_callId;
}

std::string CallRequest::file() const {
    return m_file;
}

std::string CallRequest::function() const {
    return m_function;
}

uint64_t CallRequest::executionPayment() const {
    return m_executionPayment;
}

uint64_t CallRequest::downloadPayment() const {
    return m_downloadPayment;
}

CallerKey CallRequest::callerKey() const {
    return m_callerKey;
}

uint64_t CallRequest::blockHeight() const {
    return m_blockHeight;
}

bool CallRequest::isManual() const {
    return false;
}

std::vector<uint8_t> CallRequest::arguments() const {
    return {};
}

std::vector<ServicePayment> CallRequest::servicePayments() const {
    return {};
}

}