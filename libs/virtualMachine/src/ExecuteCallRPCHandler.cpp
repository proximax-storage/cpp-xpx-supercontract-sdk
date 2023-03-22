/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ExecuteCallRPCHandler.h"
#include "CloseConnectionRPCHandler.h"
#include "CloseFileRPCHandler.h"
#include "CreateDirRPCHandler.h"
#include "CreateIteratorRPCHandler.h"
#include "DestroyIteratorRPCHandler.h"
#include "ExecuteCallRPCFinisher.h"
#include "ExecuteCallRPCReader.h"
#include "ExecuteCallRPCStarter.h"
#include "ExecuteCallRPCWriter.h"
#include "FlushRPCHandler.h"
#include "HasNextRPCHandler.h"
#include "IsFileRPCHandler.h"
#include "FileSizeRPCHandler.h"
#include "MoveFilesystemEntryRPCHandler.h"
#include "NextRPCHandler.h"
#include "OpenConnectionRPCHandler.h"
#include "OpenFileRPCHandler.h"
#include "PathExistRPCHandler.h"
#include "ReadConnectionRPCHandler.h"
#include "ReadFileRPCHandler.h"
#include "RemoveFileIteratorRPCHandler.h"
#include "RemoveFilesystemEntryRPCHandler.h"
#include "WriteFileRPCHandler.h"
#include "GetBlockHeightRPCHandler.h"
#include "GetBlockHashRPCHandler.h"
#include "GetBlockTimeRPCHandler.h"
#include "GetBlockGenerationTimeRPCHandler.h"
#include "GetTransactionHashRPCHandler.h"
#include "GetCallerPublicKeyRPCHandler.h"
#include "GetContractPublicKeyRPCHandler.h"
#include "GetServicePaymentsRPCHandler.h"
#include "GetExecutionPaymentRPCHandler.h"
#include "GetDownloadPaymentRPCHandler.h"
#include "virtualMachine/ExecutionErrorConidition.h"
#include <virtualMachine/CallRequest.h>
#include <blockchain/AggregatedTransaction.h>

namespace sirius::contract::vm {

ExecuteCallRPCHandler::ExecuteCallRPCHandler(
        GlobalEnvironment& environment,
        CallRequest&& callRequest,
        supercontractserver::SupercontractServer::Stub& stub,
        grpc::CompletionQueue& completionQueue,
        std::weak_ptr<VirtualMachineInternetQueryHandler>&& internetQueryHandler,
        std::weak_ptr<VirtualMachineBlockchainQueryHandler>&& blockchainQueryHandler,
        std::weak_ptr<VirtualMachineStorageQueryHandler>&& storageQueryHandler,
        std::shared_ptr<AsyncQueryCallback<CallExecutionResult>>

        && callback)
        :

        m_environment(environment), m_request(std::move(callRequest)), m_stream(
        stub.PrepareAsyncExecuteCall(&m_context, &completionQueue)), m_callback(std::move(callback))
        , m_internetQueryHandler(std::move(internetQueryHandler)), m_blockchainQueryHandler(
        std::move(blockchainQueryHandler)), m_storageQueryHandler(std::move(storageQueryHandler)) {}

void ExecuteCallRPCHandler::start() {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(!m_tagQuery, m_environment.logger())
    ASSERT(!m_responseHandler, m_environment.logger())

    auto[query, callback] = createAsyncQuery<void>([this](auto&& res) { onStarted(std::forward<decltype(res)>(res)); },
                                                   [] {}, m_environment, true, true);
    m_tagQuery = std::move(query);
    auto* starter = new ExecuteCallRPCStarter(m_environment, callback);
    m_stream->StartCall(starter);
}

void ExecuteCallRPCHandler::onStarted(expected<void>&& res) {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_tagQuery, m_environment.logger())
    ASSERT(!m_responseHandler, m_environment.logger())

    m_tagQuery.reset();

    if (!res) {
        postResponse(tl::unexpected(res.error()));
        return;
    }

    auto* pRpcRequest = new supercontractserver::ExecuteRequest();
    pRpcRequest->set_file_to_call(std::move(m_request.m_file));
    pRpcRequest->set_function_to_call(std::move(m_request.m_function));
    pRpcRequest->set_execution_gas_limit(m_request.m_executionGasLimit);
    pRpcRequest->set_download_gas_limit(m_request.m_downloadGasLimit);
    pRpcRequest->set_call_mode((uint32_t) m_request.m_callLevel);
    pRpcRequest->set_poex_secret_data_prefix(m_request.m_proofOfExecutionPrefix);

    supercontractserver::Request requestWrapper;
    requestWrapper.set_allocated_request(pRpcRequest);

    writeRequest(std::move(requestWrapper));
}

void ExecuteCallRPCHandler::onWritten(expected<void>&& res) {
    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_tagQuery, m_environment.logger())
    ASSERT(!m_responseHandler, m_environment.logger())

    m_tagQuery.reset();

    if (!res) {
        postResponse(tl::unexpected(res.error()));
        return;
    }

    auto[query, callback] = createAsyncQuery<supercontractserver::Response>(
            [this](auto&& response) { onRead(std::forward<decltype(response)>(response)); },
            [] {}, m_environment, true, true);
    m_tagQuery = std::move(query);
    auto* reader = new ExecuteCallRPCReader(m_environment, callback);
    m_stream->Read(&reader->m_response, reader);
}

void ExecuteCallRPCHandler::writeRequest(supercontractserver::Request&& requestWrapper) {

    ASSERT(isSingleThread(), m_environment.logger())

    auto[query, callback] = createAsyncQuery<void>([this](auto&& res) { onWritten(std::forward<decltype(res)>(res)); },
                                                   [] {},
                                                   m_environment, true, true);
    auto* writer = new ExecuteCallRPCWriter(m_environment, callback);
    m_tagQuery = std::move(query);
    m_stream->Write(requestWrapper, writer);
}

void ExecuteCallRPCHandler::processExecuteCallResponse(
        const supercontractserver::ExecuteReturns& executeCallResponse) {

    std::optional<blockchain::AggregatedTransaction> aggregatedTransaction;

    if (executeCallResponse.has_transaction()) {
        aggregatedTransaction = blockchain::AggregatedTransaction{};
        const auto& resultTransaction = executeCallResponse.transaction();
        aggregatedTransaction->m_maxFee = resultTransaction.max_fee();
        for (const auto& transaction: resultTransaction.transactions()) {
            blockchain::EmbeddedTransaction embeddedTransaction;
            embeddedTransaction.m_entityType = transaction.entity_type();
            embeddedTransaction.m_version = transaction.version();
            embeddedTransaction.m_payload = {transaction.payload().begin(), transaction.payload().end()};
            aggregatedTransaction->m_transactions.push_back(std::move(embeddedTransaction));
        }
    }

    CallExecutionResult executionResult = {
            executeCallResponse.success(),
            executeCallResponse.return_val(),
            executeCallResponse.execution_gas_consumed(),
            executeCallResponse.download_gas_consumed(),
            executeCallResponse.poex_secret_data(),
            aggregatedTransaction
    };

    m_receivedExecutionResult = true;
    postResponse(std::move(executionResult));
}

void ExecuteCallRPCHandler::onRead(expected<supercontractserver::Response>&& response) {

    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_tagQuery, m_environment.logger())
    ASSERT(!m_responseHandler, m_environment.logger())

    if (!response) {
        postResponse(tl::unexpected(response.error()));
        return;
    }

    m_tagQuery.reset();

    switch (response->server_message_case()) {
        case supercontractserver::Response::kReturns: {
            processExecuteCallResponse(response->returns());
            break;
        }
        case supercontractserver::Response::kGetBlockHeight: {
            processGetBlockHeight(response->get_block_height());
            break;
        }
        case supercontractserver::Response::kGetBlockHash: {
            processGetBlockHash(response->get_block_hash());
            break;
        }
        case supercontractserver::Response::kGetBlockTime: {
            processGetBlockTime(response->get_block_time());
            break;
        }
        case supercontractserver::Response::kGetBlockGenerationTime: {
            processGetBlockGenerationTime(response->get_block_generation_time());
            break;
        }
        case supercontractserver::Response::kGetTransactionHash: {
            processGetTransactionHash(response->get_transaction_hash());
            break;
        }
        case supercontractserver::Response::kGetCallerPublicKey: {
            processGetCallerPublicKey(response->get_caller_public_key());
            break;
        }
        case supercontractserver::Response::kGetContractPublicKey: {
            processContractPublicKey(response->get_contract_public_key());
            break;
        }
        case supercontractserver::Response::kGetServicePayments: {
            processGetServicePayments(response->get_service_payments());
            break;
        }
        case supercontractserver::Response::kGetExecutionPayment: {
            processGetExecutionPayment(response->get_execution_payment());
            break;
        }
        case supercontractserver::Response::kGetDownloadPayment: {
            processGetDownloadPayment(response->get_download_payment());
            break;
        }
        case supercontractserver::Response::kOpenFile: {
            processOpenFile(response->open_file());
            break;
        }
        case supercontractserver::Response::kWriteFileStream: {
            processWriteFile(response->write_file_stream());
            break;
        }
        case supercontractserver::Response::kFlush: {
            processFlush(response->flush());
            break;
        }
        case supercontractserver::Response::kReadFileStream: {
            processReadFile(response->read_file_stream());
            break;
        }
        case supercontractserver::Response::kCloseFile: {
            processCloseFile(response->close_file());
            break;
        }
        case supercontractserver::Response::kPathExist: {
            processPathExist(response->path_exist());
            break;
        }
        case supercontractserver::Response::kIsFile: {
            processIsFile(response->is_file());
            break;
        }
        case supercontractserver::Response::kFileSize: {
            processFileSize(response->file_size());
            break;
        }
        case supercontractserver::Response::kCreateDir: {
            processCreateDir(response->create_dir());
            break;
        }
        case supercontractserver::Response::kMoveFilesystemEntry: {
            processMoveFilesystemEntry(response->move_filesystem_entry());
            break;
        }
        case supercontractserver::Response::kRemoveFilesystemEntry: {
            processRemoveFile(response->remove_filesystem_entry());
            break;
        }
        case supercontractserver::Response::kOpenConnection: {
            processOpenInternetConnection(response->open_connection());
            break;
        }
        case supercontractserver::Response::kReadConnectionStream: {
            processReadInternetConnection(response->read_connection_stream());
            break;
        }
        case supercontractserver::Response::kCloseConnection: {
            processCloseInternetConnection(response->close_connection());
            break;
        }
        case supercontractserver::Response::kCreateDirIterator: {
            processCreateIterator(response->create_dir_iterator());
            break;
        }
        case supercontractserver::Response::kDestroyDirIterator: {
            processDestroyIterator(response->destroy_dir_iterator());
            break;
        }
        case supercontractserver::Response::kHasNextIterator: {
            processHasNext(response->has_next_iterator());
            break;
        }
        case supercontractserver::Response::kNextDirIterator: {
            processNext(response->next_dir_iterator());
            break;
        }
        case supercontractserver::Response::kRemoveDirIterator: {
            processRemoveFileIterator(response->remove_dir_iterator());
            break;
        }
        case supercontractserver::Response::SERVER_MESSAGE_NOT_SET: {
            ASSERT("Received unknown message from virtual machine", m_environment.logger());
            break;
        }

    }
}

void ExecuteCallRPCHandler::postResponse(expected<CallExecutionResult>&& result) {
    ASSERT(isSingleThread(), m_environment.logger())

    ASSERT(m_callback, m_environment.logger())

    // TODO This is a workaround that allows
    //  to reset m_callback before calling postReply
    auto callback = std::move(m_callback);
    callback->postReply(std::move(result));
}

void ExecuteCallRPCHandler::finish() {
    ASSERT(isSingleThread(), m_environment.logger())

    m_tagQuery.reset();
    m_responseHandler.reset();

    if (!m_receivedExecutionResult) {
        m_context.TryCancel();
    }

    // TODO According to grpc docs finish should not be called concurrently with other operations but for now we do not see any problems with it
    auto[query, callback] = createAsyncQuery<grpc::Status>([pThis = shared_from_this()](auto&& status) {
        ASSERT(status, pThis->m_environment.logger())
        pThis->onFinished(std::move(*status));
    }, [] {}, m_environment, false, true);
    auto* finisher = new ExecuteCallRPCFinisher(m_environment, callback);
    m_stream->Finish(&finisher->m_status, finisher);
}

void ExecuteCallRPCHandler::onFinished(grpc::Status&& status) {
    ASSERT(isSingleThread(), m_environment.logger())

    if (status.error_code()) {
        m_environment.logger().warn("Finished GRPC Call With Error: {}", status.error_message());
    } else {
    }
}

void ExecuteCallRPCHandler::processOpenInternetConnection(const supercontractserver::OpenConnection& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::OpenConnectionReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::internet_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::OpenConnectionReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_open_connection_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<OpenConnectionRPCHandler>(
            m_environment,
            request,
            m_internetQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processReadInternetConnection(const supercontractserver::ReadConnectionStream& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::ReadConnectionStreamReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::internet_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::ReadConnectionStreamReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_read_connection_stream_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<ReadConnectionRPCHandler>(
            m_environment,
            request,
            m_internetQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processCloseInternetConnection(const supercontractserver::CloseConnection& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::CloseConnectionReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::internet_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::CloseConnectionReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_close_connection_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<CloseConnectionRPCHandler>(
            m_environment,
            request,
            m_internetQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processOpenFile(const supercontractserver::OpenFile& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::OpenFileReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::OpenFileReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_open_file_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<OpenFileRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processWriteFile(const supercontractserver::WriteFileStream& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::WriteFileStreamReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::WriteFileStreamReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_write_file_stream_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<WriteFileRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processReadFile(const supercontractserver::ReadFileStream& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::ReadFileStreamReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::ReadFileStreamReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_read_file_stream_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<ReadFileRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processFlush(const supercontractserver::Flush& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::FlushReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::FlushReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_flush_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<FlushRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processCloseFile(const supercontractserver::CloseFile& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::CloseFileReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::CloseFileReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_close_file_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<CloseFileRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processPathExist(const supercontractserver::PathExist& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::PathExistReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::PathExistReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_path_exist_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<PathExistRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processFileSize(const supercontractserver::FileSize& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::FileSizeReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::FileSizeReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_file_size_return(status);
                writeRequest(std::move(requestWrapper));
                },
                [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<FileSizeRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processIsFile(const supercontractserver::IsFile& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::IsFileReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::IsFileReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_is_file_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<IsFileRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processCreateDir(const supercontractserver::CreateDir& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::CreateDirReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::CreateDirReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_create_dir_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<CreateDirRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processMoveFilesystemEntry(const supercontractserver::MoveFsEntry& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::MoveFsEntryReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::MoveFsEntryReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_move_filesystem_entry_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<MoveFilesystemEntryRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processRemoveFile(const supercontractserver::RemoveFsEntry& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::RemoveFsEntryReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::RemoveFsEntryReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_remove_filesystem_entry_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<RemoveFilesystemEntryRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processCreateIterator(const supercontractserver::CreateDirIterator& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::CreateDirIteratorReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::CreateDirIteratorReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_create_dir_iterator_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<CreateIteratorRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processHasNext(const supercontractserver::HasNextIterator& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::HasNextIteratorReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::HasNextIteratorReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_has_next_iterator_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<HasNextRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processNext(const supercontractserver::NextDirIterator& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::NextDirIteratorReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::NextDirIteratorReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_next_dir_iterator_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<NextRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processRemoveFileIterator(const supercontractserver::RemoveDirIterator& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::RemoveDirIteratorReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::RemoveDirIteratorReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_remove_dir_iterator_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<RemoveFileIteratorRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processDestroyIterator(const supercontractserver::DestroyDirIterator& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::DestroyDirIteratorReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::storage_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::DestroyDirIteratorReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_destroy_dir_iterator_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<DestroyIteratorRPCHandler>(
            m_environment,
            request,
            m_storageQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processGetBlockHeight(const supercontractserver::GetBlockHeight& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::GetBlockHeightReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::blockchain_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::GetBlockHeightReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_get_block_height_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<GetBlockHeightRPCHandler>(
            m_environment,
            request,
            m_blockchainQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processGetBlockHash(const supercontractserver::GetBlockHash& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::GetBlockHashReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::blockchain_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::GetBlockHashReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_get_block_hash_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<GetBlockHashRPCHandler>(
            m_environment,
            request,
            m_blockchainQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processGetBlockTime(const supercontractserver::GetBlockTime& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::GetBlockTimeReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::blockchain_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::GetBlockTimeReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_get_block_time_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<GetBlockTimeRPCHandler>(
            m_environment,
            request,
            m_blockchainQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processGetBlockGenerationTime(const supercontractserver::GetBlockGenerationTime& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::GetBlockGenerationTimeReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::blockchain_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::GetBlockGenerationTimeReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_get_block_generation_time_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<GetBlockGenerationTimeRPCHandler>(
            m_environment,
            request,
            m_blockchainQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processGetTransactionHash(const supercontractserver::GetTransactionHash& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::GetTransactionHashReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::blockchain_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::GetTransactionHashReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_get_transaction_hash_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<GetTransactionHashRPCHandler>(
            m_environment,
            request,
            m_blockchainQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processGetCallerPublicKey(const supercontractserver::GetCallerPublicKey& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::GetCallerPublicKeyReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::blockchain_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::GetCallerPublicKeyReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_get_caller_public_key_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<GetCallerPublicKeyRPCHandler>(
            m_environment,
            request,
            m_blockchainQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processContractPublicKey(const supercontractserver::GetContractPublicKey& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::GetContractPublicKeyReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::blockchain_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::GetContractPublicKeyReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_get_contract_public_key_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<GetContractPublicKeyRPCHandler>(
            m_environment,
            request,
            m_blockchainQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processGetExecutionPayment(const supercontractserver::GetExecutionPayment& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::GetExecutionPaymentReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::blockchain_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::GetExecutionPaymentReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_get_execution_payment_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<GetExecutionPaymentRPCHandler>(
            m_environment,
            request,
            m_blockchainQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processGetDownloadPayment(const supercontractserver::GetDownloadPayment& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::GetDownloadPaymentReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::blockchain_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::GetDownloadPaymentReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_get_download_payment_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<GetDownloadPaymentRPCHandler>(
            m_environment,
            request,
            m_blockchainQueryHandler,
            callback);

    m_responseHandler->process();
}

void ExecuteCallRPCHandler::processGetServicePayments(const supercontractserver::GetServicePayments& request) {
    auto[_, callback] = createAsyncQuery<supercontractserver::GetServicePaymentsReturn>(
            [this](auto&& res) {
                ASSERT(isSingleThread(), m_environment.logger())

                ASSERT(!m_tagQuery, m_environment.logger())
                ASSERT(m_responseHandler, m_environment.logger())

                m_responseHandler.reset();

                if (!res && res.error() == ExecutionError::blockchain_unavailable) {
                    postResponse(tl::unexpected<std::error_code>(res.error()));
                    return;
                }

                ASSERT(res, m_environment.logger())

                auto* status = new supercontractserver::GetServicePaymentsReturn(std::move(*res));
                supercontractserver::Request requestWrapper;
                requestWrapper.set_allocated_get_service_payments_return(status);
                writeRequest(std::move(requestWrapper));
            },
            [] {}, m_environment, false, false);
    m_responseHandler = std::make_unique<GetServicePaymentsRPCHandler>(
            m_environment,
            request,
            m_blockchainQueryHandler,
            callback);

    m_responseHandler->process();
}

} // namespace sirius::contract::vm