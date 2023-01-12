/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <supercontract/Identifiers.h>
#include <supercontract/AsyncQuery.h>
#include <blockchain/BlockchainErrorCode.h>

namespace sirius::contract::vm {

class VirtualMachineBlockchainQueryHandler {

public:

    virtual ~VirtualMachineBlockchainQueryHandler() = default;

//    virtual void getCaller(
//            const std::string& url,
//            std::shared_ptr<AsyncQueryCallback<CallerKey>> callback) = 0;
//
    virtual void blockHeight(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(blockchain::make_error_code(blockchain::BlockchainError::incorrect_query)));
    }

    virtual void blockHash(std::shared_ptr<AsyncQueryCallback<BlockHash>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(blockchain::make_error_code(blockchain::BlockchainError::incorrect_query)));
    }

    virtual void blockTime(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(blockchain::make_error_code(blockchain::BlockchainError::incorrect_query)));
    }

    virtual void blockGenerationTime(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(blockchain::make_error_code(blockchain::BlockchainError::incorrect_query)));
    }
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

    virtual TransactionHash releasedTransactionHash() const {
        return TransactionHash();
    };

};

}