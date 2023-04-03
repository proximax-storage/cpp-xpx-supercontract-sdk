/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "RPCExecutorClientEventHandler.h"

namespace sirius::contract::rpcExecutorClient {

namespace {

void fillInProofMessage(executor_server::Proof& proofMessage, blockchain::Proofs proof) {
    proofMessage.set_initial_batch(proof.m_initialBatch);

    auto tBuffer = proof.m_batchProof.m_T.toBytes();
    proofMessage.set_point_t(std::string(tBuffer.begin(), tBuffer.end()));
    proofMessage.set_scalar_r(std::string(proof.m_batchProof.m_r.begin(), proof.m_batchProof.m_r.end()));

    auto fBuffer = proof.m_tProof.m_F.toBytes();
    proofMessage.set_point_f(std::string(fBuffer.begin(), fBuffer.end()));
    proofMessage.set_scalar_k(std::string(proof.m_tProof.m_k.begin(), proof.m_tProof.m_k.end()));
}

}

RPCExecutorClientEventHandler::RPCExecutorClientEventHandler(ExecutorClientMessageSender& sender)
: m_sender(sender) {}

void RPCExecutorClientEventHandler::endBatchTransactionIsReady(
        const blockchain::SuccessfulEndBatchExecutionTransactionInfo& info) {
    auto* message = new executor_server::SuccessfulEndBatchTransactionIsReady();
    message->set_contract_key(std::string(info.m_contractKey.begin(), info.m_contractKey.end()));
    message->set_batch_index(info.m_batchIndex);
    message->set_automatic_executions_checked_up_to(info.m_automaticExecutionsCheckedUpTo);

    const auto& successfulBatchInfo = info.m_successfulBatchInfo;
    auto* pSuccessfulBatchInfoMessage = new executor_server::SuccessfulBatchInfo();
    pSuccessfulBatchInfoMessage->set_storage_hash(
            std::string(successfulBatchInfo.m_storageHash.begin(), successfulBatchInfo.m_storageHash.end()));
    pSuccessfulBatchInfoMessage->set_used_storage_size(successfulBatchInfo.m_usedStorageSize);
    pSuccessfulBatchInfoMessage->set_meta_files_size(successfulBatchInfo.m_metaFilesSize);
    auto verificationInfoBuffer = successfulBatchInfo.m_PoExVerificationInfo.toBytes();
    pSuccessfulBatchInfoMessage->set_proof_verification_info(std::string(verificationInfoBuffer.begin(),
                                                                         verificationInfoBuffer.end()));
    message->set_allocated_successful_batch_info(pSuccessfulBatchInfoMessage);

    for (const auto& callExecutionInfo: info.m_callsExecutionInfo) {
        auto* pCallExecutionInfoMessage = message->add_calls_execution_info();
        pCallExecutionInfoMessage->set_call_id(
                std::string(callExecutionInfo.m_callId.begin(), callExecutionInfo.m_callId.end()));
        pCallExecutionInfoMessage->set_manual(callExecutionInfo.m_manual);
        pCallExecutionInfoMessage->set_block(callExecutionInfo.m_block);
        pCallExecutionInfoMessage->set_call_execution_status(callExecutionInfo.m_callExecutionStatus);
        pCallExecutionInfoMessage->set_released_transaction(std::string(callExecutionInfo.m_releasedTransaction.begin(),
                                                                        callExecutionInfo.m_releasedTransaction.end()));
        for (const auto& participation: callExecutionInfo.m_executorsParticipation) {
            auto* pParticipation = pCallExecutionInfoMessage->add_call_executors_participation();
            pParticipation->set_sc_consumed(participation.m_scConsumed);
            pParticipation->set_sm_consumed(participation.m_smConsumed);
        }
    }

    for (const auto& proof: info.m_proofs) {
        auto* proofMessage = message->add_proofs();
        fillInProofMessage(*proofMessage, proof);
    }

    for (const auto& executorKey: info.m_executorKeys) {
        message->add_executor_keys(std::string(executorKey.begin(), executorKey.end()));
    }

    for (const auto& signature: info.m_signatures) {
        message->add_signatures(std::string(signature.begin(), signature.end()));
    }

    executor_server::ClientMessage clientMessage;
    clientMessage.set_allocated_successful_end_batch_transaction_is_ready(message);
    m_sender.sendMessage(std::move(clientMessage));
}

void RPCExecutorClientEventHandler::endBatchTransactionIsReady(
        const blockchain::UnsuccessfulEndBatchExecutionTransactionInfo& info) {
    auto* message = new executor_server::UnsuccessfulEndBatchTransactionIsReady();
    message->set_contract_key(std::string(info.m_contractKey.begin(), info.m_contractKey.end()));
    message->set_batch_index(info.m_batchIndex);
    message->set_automatic_executions_checked_up_to(info.m_automaticExecutionsCheckedUpTo);

    for (const auto& callExecutionInfo: info.m_callsExecutionInfo) {
        auto* pCallExecutionInfoMessage = message->add_calls_execution_info();
        pCallExecutionInfoMessage->set_call_id(
                std::string(callExecutionInfo.m_callId.begin(), callExecutionInfo.m_callId.end()));
        pCallExecutionInfoMessage->set_manual(callExecutionInfo.m_manual);
        pCallExecutionInfoMessage->set_block(callExecutionInfo.m_block);
        for (const auto& participation: callExecutionInfo.m_executorsParticipation) {
            auto* pParticipation = pCallExecutionInfoMessage->add_call_executors_participation();
            pParticipation->set_sc_consumed(participation.m_scConsumed);
            pParticipation->set_sm_consumed(participation.m_smConsumed);
        }
    }

    for (const auto& proof: info.m_proofs) {
        auto* proofMessage = message->add_proofs();
        fillInProofMessage(*proofMessage, proof);
    }

    for (const auto& executorKey: info.m_executorKeys) {
        message->add_executor_keys(std::string(executorKey.begin(), executorKey.end()));
    }

    for (const auto& signature: info.m_signatures) {
        message->add_signatures(std::string(signature.begin(), signature.end()));
    }

    executor_server::ClientMessage clientMessage;
    clientMessage.set_allocated_unsuccessful_end_batch_transaction_is_ready(message);
    m_sender.sendMessage(std::move(clientMessage));
}

void RPCExecutorClientEventHandler::endBatchSingleTransactionIsReady(
        const blockchain::EndBatchExecutionSingleTransactionInfo& info) {
    auto* message = new executor_server::EndBatchExecutionSingleTransactionIsReady();
    message->set_contract_key(std::string(info.m_contractKey.begin(), info.m_contractKey.end()));
    message->set_batch_index(info.m_batchIndex);

    auto* proofMessage = new executor_server::Proof();
    fillInProofMessage(*proofMessage, info.m_proofOfExecution);
    message->set_allocated_proof(proofMessage);

    executor_server::ClientMessage clientMessage;
    clientMessage.set_allocated_end_batch_single_transaction_is_ready(message);
    m_sender.sendMessage(std::move(clientMessage));
}

void RPCExecutorClientEventHandler::synchronizationSingleTransactionIsReady(
        const blockchain::SynchronizationSingleTransactionInfo& info) {
    auto* message = new executor_server::SynchronizationSingleTransactionIsReady();
    message->set_contract_key(std::string(info.m_contractKey.begin(), info.m_contractKey.end()));
    message->set_batch_index(info.m_batchIndex);

    executor_server::ClientMessage clientMessage;
    clientMessage.set_allocated_synchronization_single_transaction_is_ready(message);
    m_sender.sendMessage(std::move(clientMessage));
}

void RPCExecutorClientEventHandler::releasedTransactionsAreReady(
        const blockchain::SerializedAggregatedTransaction& transaction) {
    auto* message = new executor_server::ReleasedTransactionsAreReady();
    message->set_max_fee(transaction.m_maxFee);
    for (const auto& embeddedTransaction: transaction.m_transactions) {
        message->add_transactions(std::string(embeddedTransaction.begin(), embeddedTransaction.end()));
    }

    executor_server::ClientMessage clientMessage;
    clientMessage.set_allocated_released_transactions_are_ready(message);
    m_sender.sendMessage(std::move(clientMessage));
}

}