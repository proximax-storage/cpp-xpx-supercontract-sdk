/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <supercontract/Identifiers.h>
#include <supercontract/AsyncQuery.h>
#include <blockchain/BlockchainErrorCode.h>
#include <blockchain/AggregatedTransaction.h>
#include <supercontract/ServicePayment.h>

namespace sirius::contract::vm {

class VirtualMachineBlockchainQueryHandler {

public:

    virtual ~VirtualMachineBlockchainQueryHandler() = default;

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

    virtual void callerPublicKey(std::shared_ptr<AsyncQueryCallback<CallerKey>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(blockchain::make_error_code(blockchain::BlockchainError::incorrect_query)));
    }

    virtual void contractPublicKey(std::shared_ptr<AsyncQueryCallback<ContractKey>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(blockchain::make_error_code(blockchain::BlockchainError::incorrect_query)));
    }

    virtual void transactionHash(std::shared_ptr<AsyncQueryCallback<TransactionHash>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(blockchain::make_error_code(blockchain::BlockchainError::incorrect_query)));
    }

    virtual void servicePayments(std::shared_ptr<AsyncQueryCallback<std::vector<ServicePayment>>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(blockchain::make_error_code(blockchain::BlockchainError::incorrect_query)));
    }

    virtual void downloadPayment(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(blockchain::make_error_code(blockchain::BlockchainError::incorrect_query)));
    }

    virtual void executionPayment(std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(blockchain::make_error_code(blockchain::BlockchainError::incorrect_query)));
    }

};

}