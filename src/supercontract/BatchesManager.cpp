/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BatchesManager.h"

#include "ExecutorEnvironment.h"
#include "ContractEnvironment.h"

namespace sirius::contract {

struct DraftBatch {

    enum class AutomaticProcessStatus {
        NOT_STARTED, IN_PROCESS, FINISHED
    };

    AutomaticProcessStatus m_automaticProcessStatus = AutomaticProcessStatus::NOT_STARTED;
    std::deque<CallRequest> m_requests;
};

class DefaultBatchesManager : public BaseBatchesManager {

private:

    ContractEnvironment& m_contractEnvironment;
    ExecutorEnvironment& m_executorEnvironment;

    uint64_t m_nextBatchIndex;

    uint64_t m_storageSynchronizedNextBatchIndex = 0;

    uint64_t m_nextDraftBatchIndex = 0;

    std::optional<uint64_t> m_automaticExecutionsEnabledSince;

    std::map<CallId, uint64_t> m_callBatch;

    std::map<uint64_t, DraftBatch> m_batches;

public:

    DefaultBatchesManager( ContractEnvironment& contractEnvironment,
                           ExecutorEnvironment& executorEnvironment )
            : m_contractEnvironment( contractEnvironment )
            , m_executorEnvironment( executorEnvironment )
            {}

public:

    void addCall( const CallRequest& request ) override {

    }

    void setAutomaticExecutionsEnabledSince( std::optional<uint64_t> blockHeight ) override {

        if ( m_automaticExecutionsEnabledSince.has_value() == blockHeight.has_value() ) {
            // TODO Can have value but differ?
            return;
        }

        m_automaticExecutionsEnabledSince = blockHeight;

        if ( !m_automaticExecutionsEnabledSince ) {
            for ( auto& [_, batch]: m_batches ) {
                if ( batch.m_requests.back().m_isAutomatic ) {
                    batch.m_requests.pop_back();
                }
            }
        }
    };

    bool hasNextBatch() override {
        clearOutdatedBatches();
        return !m_batches.empty() && m_batches.begin()->second.m_automaticProcessStatus == DraftBatch::AutomaticProcessStatus::FINISHED;
    }

    Batch nextBatch() override {
        clearOutdatedBatches();

        auto batch = std::move(m_batches.extract(m_batches.begin()).mapped());

        return Batch{m_nextBatchIndex++, std::move(batch.m_requests)};
    }

    void onBlockPublished( const Block& block ) override {
        if (!m_automaticExecutionsEnabledSince || m_automaticExecutionsEnabledSince < block.m_height) {
            // Automatic Executions Are Disabled For This Block

            if ( m_batches.empty() ) {
                return;
            }

            auto batchIt = --m_batches.end();

            if ( batchIt->second.m_automaticProcessStatus == DraftBatch::AutomaticProcessStatus::NOT_STARTED ) {
                batchIt->second.m_automaticProcessStatus = DraftBatch::AutomaticProcessStatus::FINISHED;
            }
        }
        else {
            if ( m_batches.empty() || (--m_batches.end())->second.m_automaticProcessStatus !=
                                      DraftBatch::AutomaticProcessStatus::NOT_STARTED ) {
                m_batches[m_nextDraftBatchIndex++] = DraftBatch();
            }

            auto batchIt = --m_batches.end();

            CallId m_callId;
            std::string m_file;
            std::string m_function;
            std::vector<uint8_t> m_params;
            uint64_t m_scLimit;
            uint64_t m_smLimit;
            bool m_isAutomatic = false;

            m_executorEnvironment.virtualMachine().execute(CallRequest{})
        }
    }

public:

    // region virtual machine event handler

    bool onSuperContractCallExecuted( const CallExecutionResult& executionResult ) override {

        auto callIt = m_callBatch.find(executionResult.m_callId);

        if (callIt == m_callBatch.end()) {
            return false;
        }

        auto batchIt = m_batches.find(callIt->second);

        // TODO ASSERT
        //        _ASSERT( batchIt != m_batches.end() );

        if (executionResult.m_success && executionResult.m_return == 0) {
            batchIt->second.m_callRequests.push_back( CallRequest{
                executionResult.m_callId,
                "default.wasm",
                "main",
                {},
                50,
                0,
                true
            } );
        }

        if (batchIt->second.m_callRequests.empty()) {
            m_batches.erase(batchIt);
        }

        m_callBatch.erase(callIt);

        return true;
    }

    // endregion

private:

    void clearOutdatedBatches() {
        while ( !m_batches.empty() &&
                m_batches.begin()->second.m_automaticProcessed &&
                m_nextBatchIndex < m_storageSynchronizedNextBatchIndex ) {
            m_batches.erase( m_batches.begin());
            m_nextBatchIndex++;
        }
    }

};

}