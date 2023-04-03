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

namespace sirius::contract::rpcExecutorServer {

namespace bp = boost::process;

class RPCReplicatorException : public std::runtime_error {
public :

    explicit RPCReplicatorException(const std::string& what)
            : std::runtime_error(what) {}

};

RPCExecutor::RPCExecutor() {
}

RPCExecutor::~RPCExecutor() {

    m_serviceServer->Shutdown();
    m_completionQueue->Shutdown();

    if (m_completionQueueThread.joinable()) {
        m_completionQueueThread.join();
    }

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
                        const crypto::KeyPair& keyPair,
                        const std::string& rpcStorageAddress,
                        const std::string& rpcMessengerAddress,
                        const std::string& rpcBlockchainAddress,
                        const std::string& rpcVMAddress,
                        const std::string& logPath,
                        uint8_t networkIdentifier) {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(executorRPCAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(&m_service);
    m_completionQueue = builder.AddCompletionQueue();
    m_serviceServer = builder.BuildAndStart();

    m_completionQueueThread = std::thread([this] {
        waitForRPCResponse();
    });

    std::promise<bool> startPromise;
    auto barrier = startPromise.get_future();
    auto* startTag = new StartRPCTag(std::move(startPromise));

    m_service.RequestRunExecutor(&m_serverContext, &m_stream, m_completionQueue.get(), m_completionQueue.get(),
                                 startTag);

    m_childReplicatorProcess = bp::child(
            boost::dll::program_location().parent_path() / "sirius.contract.rpc_executor_client",
            m_address,
            bp::std_out > bp::null,
            bp::std_err > bp::null,
            bp::std_in < bp::null);

    auto status = barrier.wait_for(std::chrono::seconds(5));
    if (status != std::future_status::ready || !barrier.get()) {
        throw RPCReplicatorException("RPC Replicator start failed");
    }

    auto* message = new executor_server::StartExecutor();
    std::string privateKeyBuffer = {keyPair.privateKey().begin(), keyPair.privateKey().end()};
    message->set_private_key(std::move(privateKeyBuffer));
    message->set_rpc_storage_address(rpcStorageAddress);
    message->set_rpc_messenger_address(rpcMessengerAddress);
    message->set_rpc_vm_address(rpcVMAddress);
    message->set_rpc_blockchain_address(rpcBlockchainAddress);
    message->set_log_path(logPath);
    message->set_network_identifier(networkIdentifier);
    executor_server::ServerMessage serverMessage;
    serverMessage.set_allocated_start_executor(message);
    sendMessage(serverMessage);
}

void RPCExecutor::addContract(const ContractKey& key, AddContractRequest&& request) {
    auto* message = new executor_server::AddContract();
    message->set_contract_key(std::string(key.begin(), key.end()));
    message->set_drive_key(std::string(request.m_driveKey.begin(), request.m_driveKey.end()));

    for (const auto& [executorKey, info]: request.m_executors) {
        auto* executor = message->add_executors();
        executor->set_executor_key(std::string(executorKey.begin(), executorKey.end()));
        executor->set_next_batch_to_approve(info.m_nextBatchToApprove);
        executor->set_initial_batch(info.m_initialBatch);

        auto tBuffer = info.m_batchProof.m_T.toBytes();
        executor->set_point_t(std::string(tBuffer.begin(), tBuffer.end()));

        executor->set_scalar_r(std::string(info.m_batchProof.m_r.begin(), info.m_batchProof.m_r.end()));
    }

    for (const auto& [batchId, verificationInfo]: request.m_recentBatchesInformation) {
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

//    bytes call_id = 2;
//    string file = 3;
//    string function = 4;
//    uint64 execution_payment = 5;
//    uint64 download_payment = 6;
//    bytes caller_key = 7;
//    uint64 block_height = 8;
//    string arguments = 9;
//    repeated ServicePayment service_payments = 10;

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

    for(const auto& payment: request.servicePayments()) {
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
    for (const auto& [executorKey, info]: executors) {
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

void RPCExecutor::sendMessage(const executor_server::ServerMessage& message) {
    std::lock_guard<std::mutex> lock(m_sendMutex);
    std::promise<bool> promise;
    auto barrier = promise.get_future();
    auto* tag = new WriteRPCTag(std::move(promise));
    m_stream.Write(message, tag);
    auto success = barrier.get();
    if (!success) {
        throw RPCReplicatorException("RPC Replicator send failed");
    }
}

}