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

    int             m_unsuccessfulExecutionDelayMs = 10 * 1000;
    int             m_successfulExecutionDelayMs = 10 * 1000;

    uint64_t        m_autorunSCLimit = 50;

    std::string     m_autorunFile = "autorun.wasm";
    std::string     m_autorunFunction = "main";

    std::string     m_automaticExecutionFile = "default.wasm";
    std::string     m_automaticExecutionFunction = "main";

    std::string     m_rpcVirtualMachineAddress = "localhost:50051";
    std::string     m_rpcServerAddress = "localhost:50052";


    int             m_internetBufferSize = 16 * 1024;
    int             m_internetConnectionTimeoutMilliseconds = 10000;
    int             m_ocspQueryTimerMilliseconds = 500;
    int             m_ocspQueryMaxEfforts = 60;

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

    uint64_t autorunSCLimit() const {
        return m_autorunSCLimit;
    }

    void setAutorunSCLimit( uint64_t autorunScLimit ) {
        m_autorunSCLimit = autorunScLimit;
    }

    const std::string& autorunFile() const {
        return m_autorunFile;
    }

    void setAutorunFile( const std::string& autorunFile ) {
        m_autorunFile = autorunFile;
    }

    const std::string& autorunFunction() const {
        return m_autorunFunction;
    }

    void setAutorunFunction( const std::string& autorunFunction ) {
        m_autorunFunction = autorunFunction;
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

    const std::string& rpcVirtualMachineAddress() const {
        return m_rpcVirtualMachineAddress;
    }

    void setRpcVirtualMachineAddress( const std::string& rpcVirtualMachineAddress ) {
        m_rpcVirtualMachineAddress = rpcVirtualMachineAddress;
    }

    const std::string& rpcServerAddress() const {
        return m_rpcServerAddress;
    }

    void setRpcServerAddress( const std::string& rpcServerAddress ) {
        m_rpcServerAddress = rpcServerAddress;
    }

    int internetBufferSize() const {
        return m_internetBufferSize;
    }

    void setInternetBufferSize( int internetBufferSize ) {
        m_internetBufferSize = internetBufferSize;
    }

    int internetConnectionTimeoutMilliseconds() const {
        return m_internetConnectionTimeoutMilliseconds;
    }

    void setInternetConnectionTimeoutMilliseconds( int internetConnectionTimeoutMilliseconds ) {
        m_internetConnectionTimeoutMilliseconds = internetConnectionTimeoutMilliseconds;
    }

    int ocspQueryTimerMilliseconds() const {
        return m_ocspQueryTimerMilliseconds;
    }

    void setOcspQueryTimerMilliseconds( int ocspQueryTimerMilliseconds ) {
        m_ocspQueryTimerMilliseconds = ocspQueryTimerMilliseconds;
    }

    int ocspQueryMaxEfforts() const {
        return m_ocspQueryMaxEfforts;
    }

    void setOcspQueryMaxEfforts( int ocspQueryMaxEfforts ) {
        m_ocspQueryMaxEfforts = ocspQueryMaxEfforts;
    }
};

}