/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "CallRequest.h"

namespace sirius::contract {

class ManualCallRequest: public CallRequest{

private:

    std::vector<uint8_t> m_arguments;
    std::vector<ServicePayment> m_servicePayments;

public:

    ManualCallRequest(const CallId& callId,
                const std::string& file,
                const std::string& function,
                uint64_t executionPayment,
                uint64_t downloadPayment,
                const CallerKey& callerKey,
                uint64_t blockHeight,
                const std::vector<uint8_t>& arguments,
                const std::vector<ServicePayment>& servicePayments)
                : CallRequest(callId, file, function, executionPayment, downloadPayment, callerKey, blockHeight)
                , m_arguments(arguments)
                , m_servicePayments(servicePayments) {}

    bool isManual() const override;

    std::vector<uint8_t> arguments() const override;

    std::vector<ServicePayment> servicePayments() const override;

};

}