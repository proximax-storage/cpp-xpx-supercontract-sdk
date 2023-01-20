/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <executor/ManualCallRequest.h>

namespace sirius::contract {


bool ManualCallRequest::isManual() const {
    return true;
}

std::vector<uint8_t> ManualCallRequest::arguments() const {
    return m_arguments;
}

std::vector<ServicePayment> ManualCallRequest::servicePayments() const {
    return m_servicePayments;
}

}