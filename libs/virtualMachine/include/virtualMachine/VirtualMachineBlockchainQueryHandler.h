/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/Identifiers.h"

namespace sirius::contract::vm {

class VirtualMachineBlockchainQueryHandler {

public:

    virtual ~VirtualMachineBlockchainQueryHandler() = default;

//    virtual void getCaller(
//            std::function<void(CallerKey)>&& callback,
//            std::function<void()>&& terminateCallback) = 0;
//
//    virtual void getBlockHeight(std::function<void(uint64_t)>&& callback,
//                                std::function<void()>&& terminateCallback) = 0;
//
//    virtual void getBlockHash(std::function<void(BlockHash)>&& callback,
//                              std::function<void()>&& terminateCallback) = 0;
//
//    virtual void getBlockTime(std::function<void(uint64_t)>&& callback,
//                              std::function<void()>&& terminateCallback) = 0;
//
//    virtual void getBlockGenerationTime(std::function<void(uint64_t)>&& callback,
//                                        std::function<void()>&& terminateCallback) = 0;
//
//    virtual void getTransactionHash(std::function<void(TransactionHash)>&& callback,
//                                    std::function<void()>&& terminateCallback) = 0;

virtual TransactionHash releasedTransactionHash() const = 0;

};

}