/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "logging/LoggerConfig.h"

#include <common/Identifiers.h>

#include <memory>

namespace sirius::contract {

class ExecutorConfig {

private:

    int                     m_unsuccessfulExecutionDelayMs = 10 * 1000;
    int                     m_successfulExecutionDelayMs = 10 * 1000;

    uint64_t                m_autorunSCLimit = 100000;

    std::string             m_autorunFile = "autorun.wasm";
    std::string             m_autorunFunction = "main";

    // TODO Consider other default values
    uint64_t                m_maxAutorunExecutableSize = 1024U;
    uint64_t                m_maxAutomaticExecutableSize = 5U * 1024U * 1024U;
    uint64_t                m_maxManualExecutableSize = 5U * 1024U * 1024U;

    std::string             m_storagePathPrefix = "SC_DATA";

    int                     m_serviceUnavailableTimeoutMs = 5000;

    int                     m_internetBufferSize = 16 * 1024;
    int                     m_internetConnectionTimeoutMilliseconds = 10000;
    int                     m_ocspQueryTimerMilliseconds = 500;
    int                     m_ocspQueryMaxEfforts = 60;
    int                     m_maxInternetConnections = 5;

    int                     m_shareOpinionTimeoutMs = 2 * 1000 * 60;

    uint64_t                m_maxBatchesHistorySize = 10000U;

    uint64_t                m_executionPaymentToGasMultiplier = 1000000000U;
    uint64_t                m_downloadPaymentToGasMultiplier = 1000000U;

    uint8_t                 m_networkIdentifier;

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

    int serviceUnavailableTimeoutMs() const {
        return m_serviceUnavailableTimeoutMs;
    }

    void setServiceUnavailableTimeoutMs(int serviceUnavailableTimeoutMs) {
        m_serviceUnavailableTimeoutMs = serviceUnavailableTimeoutMs;
    }

    int shareOpinionTimeoutMs() const {
        return m_shareOpinionTimeoutMs;
    }

    void setShareOpinionTimeoutMs(int shareOpinionTimeoutMs) {
        m_shareOpinionTimeoutMs = shareOpinionTimeoutMs;
    }

    uint64_t maxBatchesHistorySize() const {
        return m_maxBatchesHistorySize;
    }

    void setMaxBatchesHistorySize(uint64_t maxBatchesHistorySize) {
        m_maxBatchesHistorySize = maxBatchesHistorySize;
    }

    uint64_t executionPaymentToGasMultiplier() const {
        return m_executionPaymentToGasMultiplier;
    }

    void setExecutionPaymentToGasMultiplier(uint64_t executionPaymentToGasMultiplier) {
        m_executionPaymentToGasMultiplier = executionPaymentToGasMultiplier;
    }

    uint64_t downloadPaymentToGasMultiplier() const {
        return m_downloadPaymentToGasMultiplier;
    }

    void setDownloadPaymentToGasMultiplier(uint64_t downloadPaymentToGasMultiplier) {
        m_downloadPaymentToGasMultiplier = downloadPaymentToGasMultiplier;
    }

    uint8_t networkIdentifier() const {
        return m_networkIdentifier;
    }

    void setNetworkIdentifier(uint8_t networkIdentifier) {
        m_networkIdentifier = networkIdentifier;
    }

    const std::string& storagePathPrefix() const {
        return m_storagePathPrefix;
    }

    void setStoragePathPrefix(const std::string& storagePathPrefix) {
        m_storagePathPrefix = storagePathPrefix;
    }

    uint64_t maxAutorunExecutableSize() const {
        return m_maxAutorunExecutableSize;
    }

    void setMaxAutorunExecutableSize(uint64_t maxAutorunExecutableSize) {
        m_maxAutorunExecutableSize = maxAutorunExecutableSize;
    }

    uint64_t maxAutomaticExecutableSize() const {
        return m_maxAutomaticExecutableSize;
    }

    void setMaxAutomaticExecutableSize(uint64_t maxAutomaticExecutableSize) {
        m_maxAutomaticExecutableSize = maxAutomaticExecutableSize;
    }

    uint64_t maxManualExecutableSize() const {
        return m_maxManualExecutableSize;
    }

    void setMaxManualExecutableSize(uint64_t maxManualExecutableSize) {
        m_maxManualExecutableSize = maxManualExecutableSize;
    }

    int maxInternetConnections() const {
        return m_maxInternetConnections;
    }

    void setMaxInternetConnections(int maxInternetConnections) {
        m_maxInternetConnections = maxInternetConnections;
    }
};

}