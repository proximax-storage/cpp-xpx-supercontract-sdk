/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <random>

#include "BatchesManager.h"

#include "ExecutorEnvironment.h"
#include "ContractEnvironment.h"

#include "log.h"

namespace sirius::contract {

class DefaultBatchesManager : public BaseBatchesManager {

private:

    struct DraftBatch {

        enum class BatchFormationStatus {
            MANUAL, AUTOMATIC, FINISHED
        };

        BatchFormationStatus m_batchFormationStatus = BatchFormationStatus::MANUAL;
        std::deque<CallRequest> m_requests;
    };

    struct AutorunCallInfo {
        uint64_t m_batchIndex;
        BlockHash m_blockHash;
    };

    ContractEnvironment& m_contractEnvironment;
    ExecutorEnvironment& m_executorEnvironment;

    uint64_t m_nextBatchIndex;

    uint64_t m_storageSynchronizedBatchIndex = 0;

    uint64_t m_nextDraftBatchIndex = 0;

    std::optional<uint64_t> m_automaticExecutionsEnabledSince;

    std::map<CallId, AutorunCallInfo> m_autorunCallInfos;

    std::map<uint64_t, DraftBatch> m_batches;

    const DebugInfo m_dbgInfo;

public:

    DefaultBatchesManager( uint64_t nextBatchIndex,
                           ContractEnvironment& contractEnvironment,
                           ExecutorEnvironment& executorEnvironment,
                           const DebugInfo& debugInfo )
            : m_contractEnvironment( contractEnvironment )
            , m_executorEnvironment( executorEnvironment )
            , m_nextBatchIndex( nextBatchIndex )
            , m_dbgInfo( debugInfo )
            {}

public:

    void addCall( const CallRequest& request ) override {
        if ( m_batches.empty() || (--m_batches.end())->second.m_batchFormationStatus !=
                                  DraftBatch::BatchFormationStatus::MANUAL ) {
            m_batches[m_nextDraftBatchIndex++] = DraftBatch();
        }

        auto batchIt = --m_batches.end();

        batchIt->second.m_requests.push_back(request);
    }

    void setAutomaticExecutionsEnabledSince( const std::optional<uint64_t>& blockHeight ) override {

        if ( m_automaticExecutionsEnabledSince.has_value() == blockHeight.has_value() ) {
            // TODO Can have value but differ?
            return;
        }

        m_automaticExecutionsEnabledSince = blockHeight;

        if ( !m_automaticExecutionsEnabledSince ) {
            for ( auto& [_, batch]: m_batches ) {
                if ( batch.m_requests.back().m_callLevel == CallRequest::CallLevel::AUTOMATIC ) {
                    batch.m_requests.pop_back();
                }
            }
        }
    };

    bool hasNextBatch() override {
        clearOutdatedBatches();
        return !m_batches.empty() && m_batches.begin()->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::FINISHED;
    }

    Batch nextBatch() override {
        clearOutdatedBatches();

        auto batch = std::move(m_batches.extract(m_batches.begin()).mapped());

        return Batch{m_nextBatchIndex++, std::move(batch.m_requests)};
    }

    void addBlockInfo( const Block& block ) override {
        if (!m_automaticExecutionsEnabledSince || m_automaticExecutionsEnabledSince < block.m_height) {
            // Automatic Executions Are Disabled For This Block

            if ( m_batches.empty() ) {
                return;
            }

            auto batchIt = --m_batches.end();

            if ( batchIt->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::MANUAL ) {
                batchIt->second.m_batchFormationStatus = DraftBatch::BatchFormationStatus::FINISHED;
            }
        }
        else {
            if ( m_batches.empty() || (--m_batches.end())->second.m_batchFormationStatus !=
                                      DraftBatch::BatchFormationStatus::MANUAL ) {
                m_batches[m_nextDraftBatchIndex++] = DraftBatch();
            }

            auto batchIt = --m_batches.end();
            batchIt->second.m_batchFormationStatus = DraftBatch::BatchFormationStatus::AUTOMATIC;

            CallId callId;

            std::seed_seq seed(block.m_blockHash.begin(), block.m_blockHash.end());
            std::mt19937 rng(seed);
            std::generate_n(callId.begin(), sizeof(CallId), rng);

            m_autorunCallInfos[callId] = AutorunCallInfo{batchIt->first, block.m_blockHash};

            if ( auto pVirtualMachine = m_executorEnvironment.virtualMachine().lock(); pVirtualMachine ) {
                pVirtualMachine->executeCall( m_contractEnvironment.contractKey(), CallRequest{
                    callId,
                    m_executorEnvironment.executorConfig().autorunFile(),
                    m_executorEnvironment.executorConfig().autorunFunction(),
                    {},
                    m_executorEnvironment.executorConfig().autorunSCLimit(),
                    0,
                    CallRequest::CallLevel::AUTORUN
                } );
            }
        }
    }

public:

    // region virtual machine event handler

    bool onSuperContractCallExecuted( const CallId& callId, std::optional<vm::CallExecutionResult>& executionResult ) {

        auto callIt = m_autorunCallInfos.find(callId);

        if (callIt == m_autorunCallInfos.end()) {
            return false;
        }

        auto batchIt = m_batches.find(callIt->second.m_batchIndex);

        _ASSERT( batchIt != m_batches.end() );
        _ASSERT ( batchIt->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::AUTOMATIC )

        if ( executionResult.m_success && executionResult.m_return == 0 ) {
            CallReferenceInfo info;
            info.m_callerKey = CallerKey();
            batchIt->second.m_requests.push_back( CallRequest{
                    CallId{callIt->second.m_blockHash.array()},
                    m_executorEnvironment.executorConfig().automaticExecutionFile(),
                    m_executorEnvironment.executorConfig().automaticExecutionFunction(),
                    {},
                    m_contractEnvironment.automaticExecutionsSCLimit(),
                    m_contractEnvironment.automaticExecutionsSMLimit(),
                    CallRequest::CallLevel::AUTOMATIC
            } );
        }

        batchIt->second.m_batchFormationStatus = DraftBatch::BatchFormationStatus::FINISHED;

        if (batchIt->second.m_requests.empty()) {
            m_batches.erase(batchIt);
        }

        m_autorunCallInfos.erase(callIt);

        return true;
    }

    // endregion

public:

    // region blockchain event handler

    bool onStorageSynchronized( uint64_t batchIndex ) override {
        m_storageSynchronizedBatchIndex = batchIndex;

        return true;
    }

    // endregion

private:

    void clearOutdatedBatches() {
        while ( !m_batches.empty() &&
                m_batches.begin()->second.m_batchFormationStatus == DraftBatch::BatchFormationStatus::FINISHED &&
                m_nextBatchIndex <= m_storageSynchronizedBatchIndex ) {
            m_batches.erase( m_batches.begin());
            m_nextBatchIndex++;
        }
    }

};

}