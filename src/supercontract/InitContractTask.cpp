/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BaseContractTask.h"

namespace sirius::contract {

class InitContractTask : public BaseContractTask {

private:

    const AddContractRequest m_request;

public:

    InitContractTask(
            AddContractRequest&& request,
            ContractEnvironment& contractEnvironment,
            ExecutorEnvironment& executorEnvironment )
            : BaseContractTask( executorEnvironment, contractEnvironment )
            , m_request( std::move(request) ) {}

public:

    void run() override {
        if ( !m_request.m_isContractDeployed ) {
            // The corresponding storage is not yet fully controlled by the Contract,
            // so the Storage performs necessary synchronization by himself
            m_contractEnvironment.finishTask();
        }
    }


    void terminate() override {
        m_contractEnvironment.finishTask();
    }

public:

    // region blockchain event handler

    bool onEndBatchExecutionPublished( const PublishedEndBatchExecutionTransactionInfo& info ) override {

        const auto& cosigners = info.m_cosigners;

        if ( std::find( cosigners.begin(), cosigners.end(), m_executorEnvironment.keyPair().publicKey()) != cosigners.end()) {
            // We are in the actual state from the point of blockchain view
            // At the same time storage actually may not be in the actual state
            // So we should try to synchronize storage without expecting approval in the blockchain
            m_executorEnvironment.storage().synchronizeStorage( m_contractEnvironment.driveKey(), info.m_driveState );
        }
        else {
            m_contractEnvironment.addSynchronizationTask( SynchronizationRequest{info.m_driveState} );
        }

        m_contractEnvironment.finishTask();

        return true;
    }

    // endregion

};

std::unique_ptr<BaseContractTask> createInitContractTask( AddContractRequest&& request,
                                                          ContractEnvironment& contractEnvironment,
                                                          ExecutorEnvironment& executorEnvironment ) {
    return std::make_unique<InitContractTask>(std::move(request), contractEnvironment, executorEnvironment);
}

}