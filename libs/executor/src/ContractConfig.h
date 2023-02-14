/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "executor/ExecutorConfig.h"

#include "supercontract/Identifiers.h"

#include <memory>

namespace sirius::contract {

class ContractConfig {

private:

    int m_unsuccessfulApprovalDelayMs = 2 * 60 * 60 * 1000; // 2 hours

    std::string                         m_automaticExecutionFile = "default.wasm";
    std::string                         m_automaticExecutionFunction = "main";

public:

    ContractConfig() {}

    int unsuccessfulApprovalDelayMs() const {
        return m_unsuccessfulApprovalDelayMs;
    }

    void setUnsuccessfulApprovalDelayMs( int unsuccessfulApprovalDelayMs ) {
        m_unsuccessfulApprovalDelayMs = unsuccessfulApprovalDelayMs;
    }

    const std::string& automaticExecutionFile() const {
        return m_automaticExecutionFile;
    }

    void setAutomaticExecutionFile( const std::string& automaticExecutionFile ) {
        m_automaticExecutionFile = automaticExecutionFile;
    }

    const std::string& automaticExecutionFunction() const {
        return m_automaticExecutionFunction;
    }

    void setAutomaticExecutionFunction( const std::string& automaticExecutionFunction ) {
        m_automaticExecutionFunction = automaticExecutionFunction;
    }

};

}