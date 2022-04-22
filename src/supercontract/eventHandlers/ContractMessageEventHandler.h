/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/Messages.h"

namespace sirius::contract {

class ContractMessageEventHandler {

public:

    virtual ~ContractMessageEventHandler() = default;

    virtual bool onEndBatchExecutionOpinionReceived( const EndBatchExecutionOpinion& ) {
        return false;
    }
};

}