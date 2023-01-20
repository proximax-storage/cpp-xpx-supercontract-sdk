/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <supercontract/Identifiers.h>
#include <supercontract/ServicePayment.h>

namespace sirius::contract {

class CallRequest {

private:

    CallId m_callId;
    std::string m_file;
    std::string m_function;
    uint64_t m_executionPayment;
    uint64_t m_downloadPayment;
    CallerKey m_callerKey;
    uint64_t m_blockHeight;

public:

    CallRequest(const CallId& callId,
                const std::string& file,
                const std::string& function,
                uint64_t executionPayment,
                uint64_t downloadPayment,
                const CallerKey& callerKey,
                uint64_t blockHeight);

    virtual ~CallRequest() = default;

    virtual CallId callId() const;

    virtual std::string file() const;

    virtual std::string function() const;

    virtual uint64_t executionPayment() const;

    virtual uint64_t downloadPayment() const;

    virtual CallerKey callerKey() const;

    virtual uint64_t blockHeight() const;

    virtual bool isManual() const;

    virtual std::vector<uint8_t> arguments() const;

    virtual std::vector<ServicePayment> servicePayments() const;
};

}