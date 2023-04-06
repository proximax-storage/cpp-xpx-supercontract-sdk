
/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "RPCExecutorClient.h"
#include "RPCTag.h"

#include "WriteRPCTag.h"
#include "ReadRPCTag.h"
#include "StartRPCTag.h"
#include "FinishRPCTag.h"
#include "RPCExecutorClientEventHandler.h"

#include <executor/DefaultExecutorBuilder.h>
#include <storage/RPCStorageBuilder.h>
#include <virtualMachine/RPCVirtualMachineBuilder.h>
#include <messenger/RPCMessengerBuilder.h>
#include <blockchain/Blockchain.h>
#include <crypto/KeyPair.h>

namespace sirius::contract::rpcExecutorClient {

namespace {

template<class TExecutorsBuffer>
std::map<ExecutorKey, ExecutorInfo> parseExecutorsInfo(const TExecutorsBuffer& buffer) {

    std::map<ExecutorKey, ExecutorInfo> executors;

    for (const auto& executorMessage: buffer) {
        ExecutorKey executorKey(executorMessage.executor_key());
        ExecutorInfo executorInfo;
        executorInfo.m_nextBatchToApprove = executorMessage.next_batch_to_approve();
        executorInfo.m_initialBatch = executorMessage.initial_batch();

        const auto& pointBuffer = *reinterpret_cast<const std::array<uint8_t, 32>* >(executorMessage.point_t().data());
        executorInfo.m_batchProof.m_T.fromBytes(pointBuffer);

        const auto& scalarBuffer = *reinterpret_cast<const std::array<uint8_t, 32>* >(executorMessage.scalar_r().data());
        executorInfo.m_batchProof.m_r = crypto::Scalar(scalarBuffer);
        executors.try_emplace(executorKey, std::move(executorInfo));
    }
    return executors;
}

}

RPCExecutorClient::RPCExecutorClient(const std::string& address)
        : m_stub(executor_server::ExecutorServer::NewStub(grpc::CreateChannel(
        address, grpc::InsecureChannelCredentials())))
          , m_stream(m_stub->PrepareAsyncRunExecutor(&m_context, &m_completionQueue))
          , m_blockingThread([this] {
            waitForRPCResponse();
        }) {}

RPCExecutorClient::~RPCExecutorClient() {
    if (m_blockingThread.joinable()) {
        m_blockingThread.join();
    }
}

void RPCExecutorClient::sendMessage(executor_server::ClientMessage&& request) {
    m_threadManager.execute([this, request = std::move(request)] {
        if (!m_errorOccurred && !m_outstandingWrite) {
            createWriteTag(request);
        }
    });
}

std::future<void> RPCExecutorClient::run() {
    auto* pStartTag = new StartRPCTag(*this);
    m_stream->StartCall(pStartTag);
    return m_promise.get_future();
}

void RPCExecutorClient::waitForRPCResponse() {
    void* pTag;
    bool ok;
    while (m_completionQueue.Next(&pTag, &ok)) {
        auto* pQuery = static_cast<RPCTag*>(pTag);
        pQuery->process(ok);
        delete pQuery;
    }
}

void RPCExecutorClient::onWritten(tl::expected<void, std::error_code>&& res) {
    m_threadManager.execute([this, res = std::move(res)] {
        m_outstandingWrite = false;
        if (!res || m_errorOccurred) {
            onErrorOccurred();
            return;
        }

        if (!m_requests.empty()) {
            auto request = std::move(m_requests.front());
            m_requests.pop();
            createWriteTag(request);
        }
    });
}

void RPCExecutorClient::onRead(tl::expected<executor_server::ServerMessage, std::error_code>&& res) {
    m_threadManager.execute([this, res = std::move(res)] {
        m_outstandingRead = false;
        if (!res || m_errorOccurred) {
            onErrorOccurred();
            return;
        }

        const auto& response = *res;

        switch (response.server_message_case()) {
            case executor_server::ServerMessage::kStartExecutor: {
                processStartExecutor(response.start_executor());
                break;
            }
            case executor_server::ServerMessage::kAddContract: {
                processAddContract(response.add_contract());
                break;
            }
            case executor_server::ServerMessage::kAddManualCall: {
                processAddManualCall(response.add_manual_call());
                break;
            }
            case executor_server::ServerMessage::kSetAutomaticExecutionsEnabledSince: {
                processSetAutomaticExecutionsEnabledSince(response.set_automatic_executions_enabled_since());
                break;
            }
            case executor_server::ServerMessage::kAddBlockInfo: {
                processAddBlockInfo(response.add_block_info());
                break;
            }
            case executor_server::ServerMessage::kAddBlock: {
                processAddBlock(response.add_block());
                break;
            }
            case executor_server::ServerMessage::kRemoveContract: {
                processRemoveContract(response.remove_contract());
                break;
            }
            case executor_server::ServerMessage::kSetExecutors: {
                processSetExecutors(response.set_executors());
                break;
            }
            case executor_server::ServerMessage::kPublishedEndBatchExecutionTransaction: {
                processPublishedEndBatchExecutionTransaction(response.published_end_batch_execution_transaction());
                break;
            }
            case executor_server::ServerMessage::kPublishedEndBatchExecutionSingleTransaction: {
                processPublishedEndBatchExecutionSingleTransaction(response.published_end_batch_execution_single_transaction());
                break;
            }
            case executor_server::ServerMessage::kFailedEndBatchExecution: {
                processFailedEndBatchExecution(response.failed_end_batch_execution());
                break;
            }
            case executor_server::ServerMessage::kPublishedSynchronizeSingleTransaction: {
                processPublishedSynchronizeSingleTransaction(response.published_synchronize_single_transaction());
                break;
            }
            case executor_server::ServerMessage::SERVER_MESSAGE_NOT_SET: {
                break;
            }
        }

        createReadTag();
    });
}

void RPCExecutorClient::onErrorOccurred() {
    m_errorOccurred = true;
    m_context.TryCancel();

    // This will prevent sending further messages from executor
    m_executor.reset();

    if (!hasOutstandingTags()) {
        auto* pFinishTag = new FinishRPCTag(*this);
        m_stream->Finish(&pFinishTag->m_status, pFinishTag);
    }
}

void RPCExecutorClient::createWriteTag(const executor_server::ClientMessage& request) {
    m_outstandingWrite = true;
    auto* pWriteTag = new WriteRPCTag(*this);
    m_stream->Write(request, pWriteTag);
}

bool RPCExecutorClient::hasOutstandingTags() const {
    return m_outstandingRead || m_outstandingWrite;
}

void RPCExecutorClient::onStarted(tl::expected<void, std::error_code>&& res) {
    m_threadManager.execute([this, res = std::move(res)] {
        if (!res) {
            onErrorOccurred();
            return;
        }

        createReadTag();
    });
}

void RPCExecutorClient::createReadTag() {
    m_outstandingRead = true;
    auto* readTag = new ReadRPCTag(*this);
    m_stream->Read(&readTag->m_response, readTag);
}

void RPCExecutorClient::onFinished(tl::expected<void, std::error_code>&& res) {
    m_completionQueue.Shutdown();
    m_promise.set_value();
}

void RPCExecutorClient::processStartExecutor(const executor_server::StartExecutor& response) {
    // TODO Should we set bytes in the string to zero?
    std::vector<uint8_t> keyBuffer = {response.private_key().begin(), response.private_key().end()};
    auto privateKey = std::move(*reinterpret_cast<sirius::crypto::PrivateKey*>(keyBuffer.data()));
    auto keyPair = sirius::crypto::KeyPair::FromPrivate(std::move(privateKey));

    // TODO blockchain builder
    std::unique_ptr<ServiceBuilder<blockchain::Blockchain>> blockchainBuilder;

    std::unique_ptr<ServiceBuilder<storage::Storage>> storageBuilder =
            std::make_unique<storage::RPCStorageBuilder>(response.rpc_storage_address());

    std::unique_ptr<messenger::MessengerBuilder> messengerBuilder =
            std::make_unique<messenger::RPCMessengerBuilder>(response.rpc_messenger_address());

    std::unique_ptr<vm::VirtualMachineBuilder> vmBuilder = std::make_unique<vm::RPCVirtualMachineBuilder>(
            response.rpc_vm_address());

    std::unique_ptr<ExecutorEventHandler> eventHandler = std::make_unique<RPCExecutorClientEventHandler>(*this);

    sirius::contract::ExecutorConfig config;
    config.setNetworkIdentifier(static_cast<uint8_t>(response.network_identifier()));

    logging::LoggerConfig loggerConfig;
    // TODO Maybe not?
    loggerConfig.setLogToConsole(true);
    loggerConfig.setLogPath(response.log_path());
    loggerConfig.setMaxLogFiles(10);
    loggerConfig.setMaxLogSize(50 * 1024 * 1024);

    auto logger = std::make_shared<logging::Logger>(loggerConfig, "executor");

    m_executor = sirius::contract::DefaultExecutorBuilder().build(
            std::move(keyPair),
            config,
            std::move(eventHandler),
            std::move(vmBuilder),
            std::move(storageBuilder),
            std::move(blockchainBuilder),
            std::move(messengerBuilder),
            logger);
}

void RPCExecutorClient::processAddContract(const executor_server::AddContract& message) {
    ContractKey contractKey(message.contract_key());
    AddContractRequest request;
    request.m_driveKey = DriveKey(message.drive_key());

    request.m_executors = parseExecutorsInfo(message.executors());

    for (const auto& batchMessage: message.batches()) {
        uint64_t batchId = batchMessage.batch_id();

        const auto& pointBuffer = *reinterpret_cast<const std::array<uint8_t, 32>* >(batchMessage.verification_info().data());
        crypto::CurvePoint verificationInformation;
        verificationInformation.fromBytes(pointBuffer);

        request.m_recentBatchesInformation.try_emplace(batchId, verificationInformation);
    }

    request.m_contractDeploymentBaseModificationId = ModificationId(message.contract_deployment_base_modificationid());
    request.m_automaticExecutionsFileName = message.automatic_executions_file_name();
    request.m_automaticExecutionsFunctionName = message.automatic_executions_function_name();
    request.m_automaticExecutionsSCLimit = message.automatic_executions_sc_limit();
    request.m_automaticExecutionsSMLimit = message.automatic_executions_sm_limit();

    m_executor->addContract(contractKey, std::move(request));
}

void RPCExecutorClient::processAddManualCall(const executor_server::AddManualCall& message) {
    ContractKey contractKey(message.contract_key());

    std::vector<ServicePayment> servicePayments;
    servicePayments.reserve(message.service_payments_size());

    for (const auto& paymentMessage: message.service_payments()) {
        servicePayments.push_back(ServicePayment{paymentMessage.mosaic_id(), paymentMessage.amount()});
    }

    ManualCallRequest request(
            CallId(message.call_id()),
            message.file(),
            message.function(),
            message.execution_payment(),
            message.download_payment(),
            CallerKey(message.caller_key()),
            message.block_height(),
            std::vector<uint8_t>(message.arguments().begin(), message.arguments().end()),
            servicePayments
    );

    m_executor->addManualCall(contractKey, std::move(request));
}

void RPCExecutorClient::processSetAutomaticExecutionsEnabledSince(
        const executor_server::SetAutomaticExecutionsEnabledSince& message) {
    m_executor->setAutomaticExecutionsEnabledSince(ContractKey(message.contract_key()), message.block_height());
}

void RPCExecutorClient::processAddBlockInfo(const executor_server::AddBlockInfo& message) {
    blockchain::Block block;
    block.m_blockHash = BlockHash(message.block_hash());
    block.m_blockTime = message.block_time();
    m_executor->addBlockInfo(message.block_height(), std::move(block));
}

void RPCExecutorClient::processAddBlock(const executor_server::AddBlock& message) {
    m_executor->addBlock(ContractKey(message.contract_key()), message.height());
}

void RPCExecutorClient::processRemoveContract(const executor_server::RemoveContract& message) {
    m_executor->removeContract(ContractKey(message.contract_key()), RemoveRequest{});
}

void RPCExecutorClient::processSetExecutors(const executor_server::SetExecutors& message) {
    ContractKey contractKey(message.contract_key());
    auto executors = parseExecutorsInfo(message.executors());
    m_executor->setExecutors(contractKey, std::move(executors));
}

void RPCExecutorClient::processPublishedEndBatchExecutionTransaction(
        const executor_server::PublishedEndBatchExecutionTransaction& message) {
    blockchain::PublishedEndBatchExecutionTransactionInfo info;
    info.m_contractKey = ContractKey(message.contract_key());
    info.m_batchIndex = message.batch_index();
    info.m_batchSuccess = message.batch_success();
    info.m_driveState = StorageHash(message.drive_state());
    info.m_PoExVerificationInfo.fromBytes(
            *reinterpret_cast<const std::array<uint8_t, 32>*>(message.proof_verification_info().data()));
    info.m_automaticExecutionsCheckedUpTo = message.automatic_executions_checked_up_to();
    if (message.automatic_executions_enabled()) {
        info.m_automaticExecutionsEnabledSince = message.automatic_executions_enabled_since();
    }
    for (const auto& cosigner: message.cosigners()) {
        info.m_cosigners.insert(ExecutorKey(cosigner));
    }

    m_executor->onEndBatchExecutionPublished(std::move(info));
}

void RPCExecutorClient::processPublishedEndBatchExecutionSingleTransaction(
        const executor_server::PublishedEndBatchExecutionSingleTransaction& message) {
    blockchain::PublishedEndBatchExecutionSingleTransactionInfo info;
    info.m_contractKey = ContractKey(message.contract_key());
    info.m_batchIndex = message.batch_index();

    m_executor->onEndBatchExecutionSingleTransactionPublished(std::move(info));
}

void RPCExecutorClient::processFailedEndBatchExecution(const executor_server::FailedEndBatchExecution& message) {
    blockchain::FailedEndBatchExecutionTransactionInfo info;
    info.m_contractKey = ContractKey(message.contract_key());
    info.m_batchIndex = message.batch_index();
    info.m_batchSuccess = message.batch_success();

    m_executor->onEndBatchExecutionFailed(std::move(info));
}

void RPCExecutorClient::processPublishedSynchronizeSingleTransaction(
        const executor_server::PublishedSynchronizeSingleTransaction& message) {
    blockchain::PublishedSynchronizeSingleTransactionInfo info;
    info.m_contractKey = ContractKey(message.contract_key());
    info.m_batchIndex = message.batch_index();

    m_executor->onStorageSynchronizedPublished(std::move(info));
}

}