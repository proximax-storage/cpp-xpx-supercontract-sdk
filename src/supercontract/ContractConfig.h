/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "contract/ExecutorConfig.h"

#include "types.h"

#include <memory>

namespace sirius::contract {

class ContractConfig {

private:

    const ExecutorConfig& m_executorConfig;

    int m_unsuccessfulApprovalDelayMs = 2 * 60 * 60 * 1000; // 2 hours

public:

    ContractConfig( const ExecutorConfig& executorConfig ) : m_executorConfig( executorConfig ) {}

    const ExecutorConfig& executorConfig() const {
        return m_executorConfig;
    }

    int unsuccessfulApprovalDelayMs() const {
        return m_unsuccessfulApprovalDelayMs;
    }

    void setUnsuccessfulApprovalDelayMs( int unsuccessfulApprovalDelayMs ) {
        m_unsuccessfulApprovalDelayMs = unsuccessfulApprovalDelayMs;
    }

};

}