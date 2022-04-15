#pragma once

/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

namespace sirius::contract {

class ProofOfExecution {

private:

    Hash256 m_secretData;

    Hash256 m_proofOfExecution;
    Hash256 m_proofOfExecutionVerificationInfo;

public:

    const Hash256& secretData() const {
        return m_secretData;
    }

    void setSecretData( const Hash256& secretData ) {
        m_secretData = secretData;
    }

    const Hash256& proofOfExecution() {
        return m_proofOfExecution;
    }

    const Hash256& proofOfExecutionVerificationInfo() {
        return m_proofOfExecutionVerificationInfo;
    }

    void computeProofOfExecution(const Hash256& batchId, const ExecutorKey& executorKey) {
        // TODO Compute PoEx
        m_secretData = Hash256();
    }

    void resetProofOfExecution() {

    }

};

}