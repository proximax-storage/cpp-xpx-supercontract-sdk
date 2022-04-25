/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BaseContractTask.h"

namespace sirius::contract {
class SynchronizationTask : public BaseContractTask {

private:

    SynchronizationRequest m_request;

public:

    SynchronizationTask( SynchronizationRequest&& synchronizationRequest,
                         ContractEnvironment& contractEnvironment,
                         ExecutorEnvironment& executorEnvironment)
                         : BaseContractTask( executorEnvironment, contractEnvironment )
                         , m_request( std::move(synchronizationRequest) )
                         {}

public:

    // region blockchain event handler

    bool onEndBatchExecutionPublished( const PublishedEndBatchExecutionTransactionInfo& info ) override {
        const auto& cosigners = info.m_cosigners;

        if ( std::find( cosigners.begin(), cosigners.end(), m_executorEnvironment.keyPair().publicKey()) == cosigners.end() ) {
            m_executorEnvironment.storage().synchronizeStorage( m_contractEnvironment.driveKey(), info.m_driveState );
        }

        // TODO What if we are among the cosigners? Is it possible?

        return true;
    }

    bool onStorageSynchronized( uint64_t ) override {

        m_contractEnvironment.finishTask();

        return true;
    }

    // endregion

public:

    void run() override {
        m_executorEnvironment.storage().synchronizeStorage( m_contractEnvironment.driveKey(), m_request.m_storageHash );
    }


    void terminate() override {
        m_contractEnvironment.finishTask();
    }
};

std::unique_ptr<BaseContractTask> createSynchronizationTask( SynchronizationRequest&& synchronizationRequest,
                                                             ContractEnvironment& contractEnvironment,
                                                             ExecutorEnvironment& executorEnvironment ) {
    return std::make_unique<SynchronizationTask>( std::move( synchronizationRequest ), contractEnvironment,
                                                  executorEnvironment );
}

}