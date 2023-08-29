/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <rpcExecutorServer/RPCExecutor.h>
#include <boost/dll.hpp>
#include <grpcpp/server_builder.h>
#include "StartRPCTag.h"
#include "WriteRPCTag.h"
#include "ReadRPCTag.h"

namespace sirius::contract::rpcExecutorServer {

namespace bp = boost::process;

class RPCExecutorException : public std::runtime_error {
public :

    explicit RPCExecutorException(const std::string& what)
            : std::runtime_error(what) {}

};

RPCExecutor::RPCExecutor(std::shared_ptr<ExecutorEventHandler> eventHandler,
						 std::shared_ptr<logging::Logger> logger)
	: GlobalEnvironment(std::move(logger))
	, m_eventHandler(std::move(eventHandler))
	, m_stream(&m_serverContext) {}

RPCExecutor::~RPCExecutor() {

	m_threadManager->execute([this] {
			m_readQuery.reset();
			m_serverContext.TryCancel();
			m_serviceServer->Shutdown();
			m_blockchainServer.reset();
			m_completionQueue->Shutdown();

			if (m_completionQueueThread.joinable()) {
				m_completionQueueThread.join();
			}
	});

    m_threadManager->stop();

    if (m_childReplicatorProcess.joinable()) {
        m_childReplicatorProcess.join();
    }
}

void RPCExecutor::waitForRPCResponse() {
    void* pTag;
    bool ok;
    while (m_completionQueue->Next(&pTag, &ok)) {
			auto* pQuery = static_cast<RPCTag*>(pTag);
			pQuery->process(ok);
			delete pQuery;
		}
}

void RPCExecutor::start(const std::string& executorRPCAddress,
                        std::unique_ptr<blockchain::Blockchain>&& blockchain,
                        const crypto::KeyPair& keyPair,
                        const std::string& rpcStorageAddress,
                        const std::string& rpcMessengerAddress,
                        const std::string& rpcVMAddress,
                        const std::string& logPath,
                        uint8_t networkIdentifier) {
    std::promise<bool> startPromise;
    auto barrier = startPromise.get_future();

    m_threadManager->execute([&, this, startPromise=std::move(startPromise)] () mutable {

        grpc::ServerBuilder builder;
        builder.AddListeningPort(executorRPCAddress, grpc::InsecureServerCredentials());
        builder.RegisterService(&m_service);

        m_blockchainServer =
                std::make_unique<blockchain::RPCBlockchainServer>(*this, builder, std::move(blockchain));

        m_completionQueue = builder.AddCompletionQueue();
        m_serviceServer = builder.BuildAndStart();

		m_blockchainServer->start();

        m_completionQueueThread = std::thread([this] {
            waitForRPCResponse();
        });

		auto* startTag = new StartRPCTag(std::move(startPromise));

        m_service.RequestRunExecutor(&m_serverContext, &m_stream, m_completionQueue.get(), m_completionQueue.get(),
                                     startTag);

        m_childReplicatorProcess = bp::child(
                boost::dll::program_location().parent_path() / "sirius.contract.rpc_executor_client",
                executorRPCAddress,
                bp::std_out > bp::null,
                bp::std_err > bp::null,
                bp::std_in < bp::null);
    });

    auto status = barrier.wait_for(std::chrono::seconds(5));
    if (status != std::future_status::ready || !barrier.get()) {
        throw RPCExecutorException("RPC Replicator start failed");
    }

    m_threadManager->execute([this] {
        readMessage();
    });

    auto* message = new executor_server::StartExecutor();
    std::string privateKeyBuffer = {keyPair.privateKey().begin(), keyPair.privateKey().end()};
    message->set_private_key(std::move(privateKeyBuffer));
    message->set_rpc_storage_address(rpcStorageAddress);
    message->set_rpc_messenger_address(rpcMessengerAddress);
    message->set_rpc_vm_address(rpcVMAddress);
    message->set_rpc_blockchain_address(executorRPCAddress);
    message->set_log_path(std::filesystem::absolute(logPath));
    message->set_network_identifier(networkIdentifier);
    executor_server::ServerMessage serverMessage;
    serverMessage.set_allocated_start_executor(message);
    sendMessage(serverMessage);
}

void RPCExecutor::addContract(const ContractKey& key, AddContractRequest&& request) {
    auto* message = new executor_server::AddContract();
    message->set_contract_key(std::string(key.begin(), key.end()));
    message->set_drive_key(std::string(request.m_driveKey.begin(), request.m_driveKey.end()));

    for (const auto&[executorKey, info]: request.m_executors) {
        auto* executor = message->add_executors();
        executor->set_executor_key(std::string(executorKey.begin(), executorKey.end()));
        executor->set_next_batch_to_approve(info.m_nextBatchToApprove);
        executor->set_initial_batch(info.m_initialBatch);

        auto tBuffer = info.m_batchProof.m_T.toBytes();
        executor->set_point_t(std::string(tBuffer.begin(), tBuffer.end()));

        executor->set_scalar_r(std::string(info.m_batchProof.m_r.begin(), info.m_batchProof.m_r.end()));
    }

    for (const auto&[batchId, verificationInfo]: request.m_recentBatchesInformation) {
        auto* batch = message->add_batches();
        batch->set_batch_id(batchId);

        auto verificationInfoBuffer = verificationInfo.toBytes();
        batch->set_verification_info(std::string(verificationInfoBuffer.begin(), verificationInfoBuffer.end()));
    }

    message->set_contract_deployment_base_modificationid(std::string(
            request.m_contractDeploymentBaseModificationId.begin(),
            request.m_contractDeploymentBaseModificationId.end()));
    message->set_automatic_executions_file_name(request.m_automaticExecutionsFileName);
    message->set_automatic_executions_function_name(request.m_automaticExecutionsFunctionName);
    message->set_automatic_executions_sc_limit(request.m_automaticExecutionsSCLimit);
    message->set_automatic_executions_sm_limit(request.m_automaticExecutionsSMLimit);

    executor_server::ServerMessage serverMessage;
    serverMessage.set_allocated_add_contract(message);
    sendMessage(serverMessage);
}

void RPCExecutor::addManualCall(const ContractKey& key, ManualCallRequest&& request) {
    auto* message = new executor_server::AddManualCall();

    message->set_contract_key(std::string(key.begin(), key.end()));

    auto callId = request.callId();
    message->set_call_id(std::string(callId.begin(), callId.end()));

    message->set_file(request.file());
    message->set_function(request.function());
    message->set_execution_payment(request.executionPayment());
    message->set_download_payment(request.downloadPayment());

    auto callerKey = request.callerKey();
    message->set_caller_key(std::string(callerKey.begin(), callerKey.end()));

    message->set_block_height(request.blockHeight());

    auto arguments = request.arguments();
    message->set_arguments(std::string(arguments.begin(), arguments.end()));

    for (const auto& payment: request.servicePayments()) {
        auto* paymentMessage = message->add_service_payments();
        paymentMessage->set_mosaic_id(payment.m_mosaicId);
        paymentMessage->set_amount(payment.m_amount);
    }

    executor_server::ServerMessage serverMessage;
    serverMessage.set_allocated_add_manual_call(message);
    sendMessage(serverMessage);
}

void RPCExecutor::setAutomaticExecutionsEnabledSince(const ContractKey& contractKey, uint64_t blockHeight) {
    auto* message = new executor_server::SetAutomaticExecutionsEnabledSince();
    message->set_contract_key(std::string(contractKey.begin(), contractKey.end()));
    message->set_block_height(blockHeight);

    executor_server::ServerMessage serverMessage;
    serverMessage.set_allocated_set_automatic_executions_enabled_since(message);
    sendMessage(serverMessage);
}

void RPCExecutor::addBlockInfo(uint64_t blockHeight, blockchain::Block&& block) {
    auto* message = new executor_server::AddBlockInfo();
    message->set_block_height(blockHeight);
    message->set_block_hash(std::string(block.m_blockHash.begin(), block.m_blockHash.end()));
    message->set_block_time(block.m_blockTime);

    executor_server::ServerMessage serverMessage;
    serverMessage.set_allocated_add_block_info(message);
    sendMessage(serverMessage);
}

void RPCExecutor::addBlock(const ContractKey& contractKey, uint64_t height) {
    auto* message = new executor_server::AddBlock();
    message->set_contract_key(std::string(contractKey.begin(), contractKey.end()));
    message->set_height(height);

    executor_server::ServerMessage serverMessage;
    serverMessage.set_allocated_add_block(message);
    sendMessage(serverMessage);
}

void RPCExecutor::removeContract(const ContractKey& key, RemoveRequest&& request) {
    auto* message = new executor_server::RemoveContract();
    message->set_contract_key(std::string(key.begin(), key.end()));

    executor_server::ServerMessage serverMessage;
    serverMessage.set_allocated_remove_contract(message);
    sendMessage(serverMessage);
}

void RPCExecutor::setExecutors(const ContractKey& key, std::map<ExecutorKey, ExecutorInfo>&& executors) {
    auto* message = new executor_server::SetExecutors();
    message->set_contract_key(std::string(key.begin(), key.end()));
    for (const auto&[executorKey, info]: executors) {
        auto* executor = message->add_executors();
        executor->set_executor_key(std::string(executorKey.begin(), executorKey.end()));
        executor->set_next_batch_to_approve(info.m_nextBatchToApprove);
        executor->set_initial_batch(info.m_initialBatch);

        auto tBuffer = info.m_batchProof.m_T.toBytes();
        executor->set_point_t(std::string(tBuffer.begin(), tBuffer.end()));

        executor->set_scalar_r(std::string(info.m_batchProof.m_r.begin(), info.m_batchProof.m_r.end()));
    }

    executor_server::ServerMessage serverMessage;
    serverMessage.set_allocated_set_executors(message);
    sendMessage(serverMessage);
}

void RPCExecutor::onEndBatchExecutionPublished(blockchain::PublishedEndBatchExecutionTransactionInfo&& info) {
    auto* message = new executor_server::PublishedEndBatchExecutionTransaction();
    message->set_contract_key(std::string(info.m_contractKey.begin(), info.m_contractKey.end()));
    message->set_batch_index(info.m_batchIndex);
    message->set_batch_success(info.m_batchSuccess);
    message->set_drive_state(std::string(info.m_driveState.begin(), info.m_driveState.end()));

    auto verificationInfoBuffer = info.m_PoExVerificationInfo.toBytes();
    message->set_proof_verification_info(
            std::string(verificationInfoBuffer.begin(), verificationInfoBuffer.end()));

    message->set_automatic_executions_checked_up_to(info.m_automaticExecutionsCheckedUpTo);
    if (info.m_automaticExecutionsEnabledSince) {
        message->set_automatic_executions_enabled(true);
        message->set_automatic_executions_enabled_since(*info.m_automaticExecutionsEnabledSince);
    } else {
        message->set_automatic_executions_enabled(false);
    }

    for (const auto& cosigner: info.m_cosigners) {
        message->add_cosigners(std::string(cosigner.begin(), cosigner.end()));
    }

    executor_server::ServerMessage serverMessage;
    serverMessage.set_allocated_published_end_batch_execution_transaction(message);
    sendMessage(serverMessage);
}

void RPCExecutor::onEndBatchExecutionSingleTransactionPublished(
        blockchain::PublishedEndBatchExecutionSingleTransactionInfo&& info) {
    auto* message = new executor_server::PublishedEndBatchExecutionSingleTransaction();
    message->set_contract_key(std::string(info.m_contractKey.begin(), info.m_contractKey.end()));
    message->set_batch_index(info.m_batchIndex);

    executor_server::ServerMessage serverMessage;
    serverMessage.set_allocated_published_end_batch_execution_single_transaction(message);
    sendMessage(serverMessage);
}

void RPCExecutor::onEndBatchExecutionFailed(blockchain::FailedEndBatchExecutionTransactionInfo&& info) {
    auto* message = new executor_server::FailedEndBatchExecution();
    message->set_contract_key(std::string(info.m_contractKey.begin(), info.m_contractKey.end()));
    message->set_batch_index(info.m_batchIndex);
    message->set_batch_success(info.m_batchSuccess);

    executor_server::ServerMessage serverMessage;
    serverMessage.set_allocated_failed_end_batch_execution(message);
    sendMessage(serverMessage);
}

void RPCExecutor::onStorageSynchronizedPublished(blockchain::PublishedSynchronizeSingleTransactionInfo&& info) {
    auto* message = new executor_server::PublishedSynchronizeSingleTransaction();
    message->set_contract_key(std::string(info.m_contractKey.begin(), info.m_contractKey.end()));
    message->set_batch_index(info.m_batchIndex);

    executor_server::ServerMessage serverMessage;
    serverMessage.set_allocated_published_synchronize_single_transaction(message);
    sendMessage(serverMessage);
}

void RPCExecutor::readMessage() {
    auto[query, callback] = createAsyncQuery<executor_server::ClientMessage>([this](auto&& res) {
        onRead(res);
    }, [] {}, *this, true, true);
    m_readQuery = std::move(query);
    auto* tag = new ReadRPCTag(std::move(callback));
    m_stream.Read(&tag->m_clientMessage, tag);
}

void RPCExecutor::onRead(const expected<executor_server::ClientMessage>& message) {

    if (!message) {
        return;
    }

    readMessage();

    switch (message->client_message_case()) {
        case executor_server::ClientMessage::kSuccessfulEndBatchTransactionIsReady:
            processSuccessfulEndBatchTransactionIsReadyMessage(
                    message->successful_end_batch_transaction_is_ready());
            break;
        case executor_server::ClientMessage::kUnsuccessfulEndBatchTransactionIsReady:
            processUnsuccessfulEndBatchTransactionIsReadyMessage(
                    message->unsuccessful_end_batch_transaction_is_ready());
            break;
        case executor_server::ClientMessage::kEndBatchSingleTransactionIsReady:
            processEndBatchExecutionSingleTransactionIsReadyMessage(
                    message->end_batch_single_transaction_is_ready());
            break;
        case executor_server::ClientMessage::kSynchronizationSingleTransactionIsReady:
            processSynchronizationSingleTransactionIsReadyMessage(
                    message->synchronization_single_transaction_is_ready());
            break;
        case executor_server::ClientMessage::kReleasedTransactionsAreReady:
            processReleasedTransactionsAreReadyMessage(message->released_transactions_are_ready());
            break;
        case executor_server::ClientMessage::CLIENT_MESSAGE_NOT_SET:
            break;
    }
}

void RPCExecutor::sendMessage(const executor_server::ServerMessage& message) {
    std::lock_guard<std::mutex> lock(m_sendMutex);
    std::promise<bool> promise;
    auto barrier = promise.get_future();
    auto* tag = new WriteRPCTag(std::move(promise));
    m_stream.Write(message, tag);
    auto success = barrier.get();
    if (!success) {
        throw RPCExecutorException("RPC Replicator send failed");
    }
}

void RPCExecutor::processSuccessfulEndBatchTransactionIsReadyMessage(
        const executor_server::SuccessfulEndBatchTransactionIsReady& message) {
    blockchain::SuccessfulEndBatchExecutionTransactionInfo info;
    info.m_contractKey = ContractKey(message.contract_key());
    info.m_batchIndex = message.batch_index();
    info.m_automaticExecutionsCheckedUpTo = message.automatic_executions_checked_up_to();

    const auto& successfulBatchInfoMessage = message.successful_batch_info();
    auto& successfulBatchInfo = info.m_successfulBatchInfo;
    successfulBatchInfo.m_storageHash = StorageHash(successfulBatchInfoMessage.storage_hash());
    successfulBatchInfo.m_usedStorageSize = successfulBatchInfoMessage.used_storage_size();
    successfulBatchInfo.m_metaFilesSize = successfulBatchInfoMessage.meta_files_size();
    successfulBatchInfo.m_PoExVerificationInfo.fromBytes(
            *reinterpret_cast<const std::array<uint8_t, 32>*>(successfulBatchInfoMessage.proof_verification_info().data()));

    info.m_callsExecutionInfo.reserve(message.calls_execution_info_size());
    for (const auto& callMessage: message.calls_execution_info()) {
        blockchain::SuccessfulCallExecutionInfo callExecutionInfo;
        callExecutionInfo.m_callId = CallId(callMessage.call_id());
        callExecutionInfo.m_manual = callMessage.manual();
        callExecutionInfo.m_block = callMessage.block();
        callExecutionInfo.m_callExecutionStatus = callMessage.call_execution_status();
        callExecutionInfo.m_releasedTransaction = callMessage.released_transaction();
        callExecutionInfo.m_executorsParticipation.reserve(callMessage.call_executors_participation_size());
        for (const auto& executorParticipationMessage: callMessage.call_executors_participation()) {
            callExecutionInfo.m_executorsParticipation.push_back(blockchain::CallExecutorParticipation{
                    executorParticipationMessage.sc_consumed(),
                    executorParticipationMessage.sm_consumed()
            });
        }
        info.m_callsExecutionInfo.push_back(callExecutionInfo);
    }

    info.m_proofs.reserve(message.proofs_size());
    for (const auto& proofMessage: message.proofs()) {

        blockchain::TProof tProof;
        tProof.m_F.fromBytes(*reinterpret_cast<const std::array<uint8_t, 32>*>(proofMessage.point_f().data()));
        tProof.m_k = crypto::Scalar(
                *reinterpret_cast<const std::array<uint8_t, 32>*>(proofMessage.scalar_k().data()));

        blockchain::BatchProof batchProof;
        batchProof.m_T.fromBytes(*reinterpret_cast<const std::array<uint8_t, 32>*>(proofMessage.point_t().data()));
        batchProof.m_r = crypto::Scalar(
                *reinterpret_cast<const std::array<uint8_t, 32>*>(proofMessage.scalar_r().data()));

        info.m_proofs.push_back(blockchain::Proofs{
                proofMessage.initial_batch(),
                tProof,
                batchProof
        });
    }

    info.m_executorKeys.reserve(message.executor_keys_size());
    for (const auto& executorKeyMessage: message.executor_keys()) {
        info.m_executorKeys.emplace_back(executorKeyMessage);
    }

    info.m_signatures.reserve(message.signatures_size());
    for (const auto& signatureMessage: message.signatures()) {
        info.m_signatures.push_back(*reinterpret_cast<const Signature*>(signatureMessage.data()));
    }

    m_eventHandler->endBatchTransactionIsReady(info);
}

void RPCExecutor::processUnsuccessfulEndBatchTransactionIsReadyMessage(
        const executor_server::UnsuccessfulEndBatchTransactionIsReady& message) {
    blockchain::UnsuccessfulEndBatchExecutionTransactionInfo info;
    info.m_contractKey = ContractKey(message.contract_key());
    info.m_batchIndex = message.batch_index();
    info.m_automaticExecutionsCheckedUpTo = message.automatic_executions_checked_up_to();

    info.m_callsExecutionInfo.reserve(message.calls_execution_info_size());
    for (const auto& callMessage: message.calls_execution_info()) {
        blockchain::UnsuccessfulCallExecutionInfo callExecutionInfo;
        callExecutionInfo.m_callId = CallId(callMessage.call_id());
        callExecutionInfo.m_manual = callMessage.manual();
        callExecutionInfo.m_block = callMessage.block();
        callExecutionInfo.m_executorsParticipation.reserve(callMessage.call_executors_participation_size());
        for (const auto& executorParticipationMessage: callMessage.call_executors_participation()) {
            callExecutionInfo.m_executorsParticipation.push_back(blockchain::CallExecutorParticipation{
                    executorParticipationMessage.sc_consumed(),
                    executorParticipationMessage.sm_consumed()
            });
        }
        info.m_callsExecutionInfo.push_back(callExecutionInfo);
    }

    info.m_proofs.reserve(message.proofs_size());
    for (const auto& proofMessage: message.proofs()) {

        blockchain::TProof tProof;
        tProof.m_F.fromBytes(*reinterpret_cast<const std::array<uint8_t, 32>*>(proofMessage.point_f().data()));
        tProof.m_k = crypto::Scalar(
                *reinterpret_cast<const std::array<uint8_t, 32>*>(proofMessage.scalar_k().data()));

        blockchain::BatchProof batchProof;
        batchProof.m_T.fromBytes(*reinterpret_cast<const std::array<uint8_t, 32>*>(proofMessage.point_t().data()));
        batchProof.m_r = crypto::Scalar(
                *reinterpret_cast<const std::array<uint8_t, 32>*>(proofMessage.scalar_r().data()));

        info.m_proofs.push_back(blockchain::Proofs{
                proofMessage.initial_batch(),
                tProof,
                batchProof
        });
    }

    info.m_executorKeys.reserve(message.executor_keys_size());
    for (const auto& executorKeyMessage: message.executor_keys()) {
        info.m_executorKeys.emplace_back(executorKeyMessage);
    }

    info.m_signatures.reserve(message.signatures_size());
    for (const auto& signatureMessage: message.signatures()) {
        info.m_signatures.push_back(*reinterpret_cast<const Signature*>(signatureMessage.data()));
    }

    m_eventHandler->endBatchTransactionIsReady(info);
}

void RPCExecutor::processEndBatchExecutionSingleTransactionIsReadyMessage(
        const executor_server::EndBatchExecutionSingleTransactionIsReady& message) {
    blockchain::EndBatchExecutionSingleTransactionInfo info;
    info.m_contractKey = ContractKey(message.contract_key());
    info.m_batchIndex = message.batch_index();
    blockchain::TProof tProof;
    tProof.m_F.fromBytes(*reinterpret_cast<const std::array<uint8_t, 32>*>(message.proof().point_f().data()));
    tProof.m_k = crypto::Scalar(
            *reinterpret_cast<const std::array<uint8_t, 32>*>(message.proof().scalar_k().data()));

    blockchain::BatchProof batchProof;
    batchProof.m_T.fromBytes(*reinterpret_cast<const std::array<uint8_t, 32>*>(message.proof().point_t().data()));
    batchProof.m_r = crypto::Scalar(
            *reinterpret_cast<const std::array<uint8_t, 32>*>(message.proof().scalar_r().data()));
    info.m_proofOfExecution = blockchain::Proofs{
            message.proof().initial_batch(),
            tProof,
            batchProof
    };

    m_eventHandler->endBatchSingleTransactionIsReady(info);
}

void RPCExecutor::processSynchronizationSingleTransactionIsReadyMessage(
        const executor_server::SynchronizationSingleTransactionIsReady& message) {
    blockchain::SynchronizationSingleTransactionInfo info;
    info.m_contractKey = ContractKey(message.contract_key());
    info.m_batchIndex = message.batch_index();

    m_eventHandler->synchronizationSingleTransactionIsReady(info);
}

void
RPCExecutor::processReleasedTransactionsAreReadyMessage(
        const executor_server::ReleasedTransactionsAreReady& message) {
    blockchain::SerializedAggregatedTransaction info;
    info.m_maxFee = message.max_fee();
    info.m_transactions.reserve(message.transactions_size());
    for (const auto& transactionMessage: info.m_transactions) {
        info.m_transactions.push_back({transactionMessage.begin(), transactionMessage.end()});
    }

    m_eventHandler->releasedTransactionsAreReady(info);
}

}