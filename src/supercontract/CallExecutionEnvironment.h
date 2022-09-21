/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/Identifiers.h"

#include "supercontract/AsyncQuery.h"

#include <virtualMachine/VirtualMachineInternetQueryHandler.h>
#include <virtualMachine/VirtualMachineBlockchainQueryHandler.h>
#include "internet/InternetConnection.h"
#include "ExecutorEnvironment.h"
#include "ContractEnvironment.h"

namespace sirius::contract {

class CallExecutionEnvironment
        : private SingleThread
        , public vm::VirtualMachineInternetQueryHandler
        , public vm::VirtualMachineBlockchainQueryHandler {

private:

    ExecutorEnvironment& m_executorEnvironment;
    ContractEnvironment& m_contractEnvironment;

    const CallRequest m_callRequest;

    std::shared_ptr<AsyncQuery> m_asyncQuery;

    uint64_t totalConnectionsCreated = 0;
    std::map<uint64_t, internet::InternetConnection> m_internetConnections;

    bool m_terminated = false;

public:

    CallExecutionEnvironment(const CallRequest& request,
                             ExecutorEnvironment& executorEnvironment,
                             ContractEnvironment& contractEnvironment);

//    const CallId& callId() const {
//
//        DBG_MAIN_THREAD_DEPRECATED
//
//        return m_callRequest.m_callId;
//    }

//    const CallRequest& callRequest() const {
//
//        DBG_MAIN_THREAD_DEPRECATED
//
//        return m_callRequest;
//    }
//
//    const CallExecutionResult& callExecutionResult() const {
//
//        DBG_MAIN_THREAD_DEPRECATED
//
//        return *m_callExecutionResult;
//    }

//    void setCallExecutionResult(const CallExecutionResult& callExecutionResult) {
//
//        DBG_MAIN_THREAD_DEPRECATED
//
//        m_callExecutionResult = callExecutionResult;
//    }

    // region internet

    void
    openConnection(const std::string& url,
                   std::shared_ptr<AsyncQueryCallback<std::optional<uint64_t>>> callback) override;

    void read(uint64_t connectionId,
              std::shared_ptr<AsyncQueryCallback<std::optional<std::vector<uint8_t>>>> callback) override;

    void closeConnection(uint64_t connectionId,
                         std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

//    // endregion
//
//    // region blockchain
//
//    void getCaller(std::function<void(CallerKey)>&& callback, std::function<void()>&& terminateCallback) override {
//
//        DBG_MAIN_THREAD_DEPRECATED
//
//        _ASSERT(m_callRequest.m_callLevel == CallRequest::CallLevel::AUTOMATIC ||
//                m_callRequest.m_callLevel == CallRequest::CallLevel::MANUAL)
//
//        _ASSERT(m_callRequest.m_referenceInfo.m_callerKey)
//
//        CallerKey callerKey = *m_callRequest.m_referenceInfo.m_callerKey;
//        callback(std::move(callerKey));
//    }
//
//    void
//    getBlockHeight(std::function<void(uint64_t)>&& callback, std::function<void()>&& terminateCallback) override {
//
//        DBG_MAIN_THREAD_DEPRECATED
//
//        callback(m_callRequest.m_referenceInfo.m_blockHeight);
//    }
//
//    void
//    getBlockHash(std::function<void(BlockHash)>&& callback, std::function<void()>&& terminateCallback) override {
//
//        DBG_MAIN_THREAD_DEPRECATED
//
//        callback(m_callRequest.m_referenceInfo.m_blockHash);
//    }
//
//    void
//    getBlockTime(std::function<void(uint64_t)>&& callback, std::function<void()>&& terminateCallback) override {
//
//        DBG_MAIN_THREAD_DEPRECATED
//
//        callback(m_callRequest.m_referenceInfo.m_blockTime);
//    }
//
//    void getBlockGenerationTime(std::function<void(uint64_t)>&& callback,
//                                std::function<void()>&& terminateCallback) override {
//
//        DBG_MAIN_THREAD_DEPRECATED
//
//        callback(m_callRequest.m_referenceInfo.m_blockGenerationTime);
//    }
//
//    void getTransactionHash(std::function<void(TransactionHash)>&& callback,
//                            std::function<void()>&& terminateCallback) override {
//
//        DBG_MAIN_THREAD_DEPRECATED
//
//        _ASSERT(m_callRequest.m_callLevel == CallRequest::CallLevel::MANUAL)
//        _ASSERT(m_callRequest.m_referenceInfo.m_transactionHash)
//
//        callback(*m_callRequest.m_referenceInfo.m_transactionHash);
//    }
//
//    // endregion

};

}