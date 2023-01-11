/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "ContractEnvironment.h"
#include "ExecutorEnvironment.h"
#include "internet/InternetConnection.h"
#include "supercontract/AsyncQuery.h"
#include "supercontract/Identifiers.h"
#include <virtualMachine/VirtualMachineBlockchainQueryHandler.h>
#include <virtualMachine/VirtualMachineInternetQueryHandler.h>
#include <virtualMachine/VirtualMachineStorageQueryHandler.h>

namespace sirius::contract {

class CallExecutionEnvironment
    : private SingleThread,
      public vm::VirtualMachineInternetQueryHandler,
      public vm::VirtualMachineBlockchainQueryHandler,
      public vm::VirtualMachineStorageQueryHandler {

private:
    ExecutorEnvironment& m_executorEnvironment;
    ContractEnvironment& m_contractEnvironment;

    const vm::CallRequest m_callRequest;

    std::shared_ptr<AsyncQuery> m_asyncQuery;

    uint64_t totalConnectionsCreated = 0;
    std::map<uint64_t, internet::InternetConnection> m_internetConnections;

    bool m_terminated = false;

public:
    CallExecutionEnvironment(const vm::CallRequest& request,
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
                   std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void read(uint64_t connectionId,
              std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) override;

    void closeConnection(uint64_t connectionId,
                         std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    //    // endregion
    //
        // region blockchain
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

    TransactionHash releasedTransactionHash() const override;

    // endregion

    void openFile(const std::string& path, const std::string& mode,
                  std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void writeFile(uint64_t fileId, const std::vector<uint8_t>& buffer,
                   std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void readFile(uint64_t fileId,
                  std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) override;

    void flush(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void closeFile(uint64_t fileId, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void createFSIterator(const std::string& path, bool recursive,
                          std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

    void hasNextIterator(uint64_t iteratorID, std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void nextIterator(uint64_t iteratorID, std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) override;

    void removeFileIterator(uint64_t iteratorID, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void destroyFSIterator(uint64_t iteratorID, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void pathExist(const std::string& path, std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void isFile(const std::string& path, std::shared_ptr<AsyncQueryCallback<bool>> callback) override;

    void createDir(const std::string& path, std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void moveFile(const std::string& oldPath, const std::string& newPath,
                  std::shared_ptr<AsyncQueryCallback<void>> callback) override;

    void removeFsEntry(const std::string& path, std::shared_ptr<AsyncQueryCallback<void>> callback) override;
};

} // namespace sirius::contract