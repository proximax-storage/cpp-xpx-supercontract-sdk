/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "Requests.h"

#include "types.h"

#include <memory>

namespace sirius::contract {

class ExecutorConfig {

private:

    int m_unsuccessfulExecutionDelayMs = 10 * 1000;
    int m_successfulExecutionDelayMs = 10 * 1000;

public:

    int unsuccessfulExecutionDelayMs() const {
        return m_unsuccessfulExecutionDelayMs;
    }

    void setUnsuccessfulExecutionDelayMs( int unsuccessfulExecutionDelayMs ) {
        m_unsuccessfulExecutionDelayMs = unsuccessfulExecutionDelayMs;
    }

    int successfulExecutionDelayMs() const {
        return m_successfulExecutionDelayMs;
    }

    void setSuccessfulExecutionDelayMs( int successfulExecutionDelayMs ) {
        m_successfulExecutionDelayMs = successfulExecutionDelayMs;
    }
};

}