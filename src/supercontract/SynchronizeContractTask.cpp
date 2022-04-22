/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "BaseContractTask.h"

namespace sirius::contract {
class SynchronizeContractTask : public BaseContractTask {

public:

    SynchronizeContractTask( Hash256& rootHash,
                             TaskContext& context )
            : BaseContractTask( context ) {
        m_taskContext.storageBridge().synchronizeStorage( m_taskContext.driveKey(), rootHash );
    }

public:

    // region blockchain event handler

    bool onEndBatchExecutionPublished( const PublishedEndBatchExecutionTransactionInfo& info ) override {
        const auto& cosigners = info.m_cosigners;

        if ( std::find( cosigners.begin(), cosigners.end(), m_taskContext.keyPair().publicKey()) != cosigners.end()) {
            // We are among the cosigners, so now  we are in the actual state and can execute usual batches
            m_taskContext.storageBridge().cancelStorageSynchronization( m_taskContext.driveKey() );
        }
        else if (info.m_driveState) {
            // TODO What if not success? BatchId?
            m_taskContext.storageBridge().synchronizeStorage( m_taskContext.driveKey(), *info.m_driveState);
        }

        return true;
    }

    bool onEndBatchExecutionSingleTransactionPublished(
            const PublishedEndBatchExecutionSingleTransactionInfo& info ) override {
        m_taskContext.storageBridge().cancelStorageSynchronization( m_taskContext.driveKey() );

        return true;
    }

    // endregion

public:

    // region storage bridge event handler

    bool onStorageSynchronized( uint64_t batchIndex ) override {
        m_taskContext.onTaskFinished();

        return true;
    }

    bool onStorageSynchronizationCancelled() override {
        m_taskContext.onTaskFinished();

        return true;
    }

    // endregion

public:

    void run() override {

        // TODO
//        m_taskContext.storageBridge().synchronizeStorage( m_taskContext.driveKey(), );
    }


    void terminate() override {
        m_taskContext.finishTask();
    }
};
}