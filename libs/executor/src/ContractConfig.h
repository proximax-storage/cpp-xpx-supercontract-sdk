/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "executor/ExecutorConfig.h"

#include <common/Identifiers.h>

#include <memory>

namespace sirius::contract {

class ContractConfig {

private:

    int m_unsuccessfulApprovalDelayMs = 10 * 60 * 60; // 10 minutes

    std::string                         m_automaticExecutionsFile = "default.wasm";
    std::string                         m_automaticExecutionsFunction = "main";

public:

    ContractConfig() {}

    int unsuccessfulApprovalDelayMs() const {
        return m_unsuccessfulApprovalDelayMs;
    }

    void setUnsuccessfulApprovalDelayMs( int unsuccessfulApprovalDelayMs ) {
        m_unsuccessfulApprovalDelayMs = unsuccessfulApprovalDelayMs;
    }

    const std::string& automaticExecutionsFile() const {
        return m_automaticExecutionsFile;
    }

    void setAutomaticExecutionsFile( const std::string& automaticExecutionsFile ) {
        m_automaticExecutionsFile = automaticExecutionsFile;
    }

    const std::string& automaticExecutionsFunction() const {
        return m_automaticExecutionsFunction;
    }

    void setAutomaticExecutionsFunction( const std::string& automaticExecutionsFunction ) {
        m_automaticExecutionsFunction = automaticExecutionsFunction;
    }

};

}