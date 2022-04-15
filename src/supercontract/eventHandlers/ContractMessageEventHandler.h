/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "contract/StorageQueries.h"

namespace sirius::contract {

class ContractMessageEventHandler {

public:

    virtual ~ContractMessageEventHandler() = default;

    //    virtual void onAppliedStorageModifications(const ContractKey& contractKey, )

    virtual bool onEndBatchExecutionOpinionReceived( const EndBatchExecutionTransactionInfo& ) {
        return false;
    }
};

}