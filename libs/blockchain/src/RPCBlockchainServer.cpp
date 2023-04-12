//
// Created by kyrylo on 07.04.2023.
//

#include "blockchain/RPCBlockchainServer.h"
#include "AcceptRequestTag.h"
#include "FinishRequestTag.h"
#include "blockchain/BlockContext.h"
#include "ServerRPCTag.h"
#include <grpcpp/server_builder.h>

namespace sirius::contract::blockchain {

RPCBlockchainServer::RPCBlockchainServer(GlobalEnvironment& environment,
                                         grpc::ServerBuilder& builder,
                                         std::unique_ptr<blockchain::Blockchain>&& blockchain)
                                         : m_environment(environment)
                                         , m_blockchain(std::move(blockchain)) {
    builder.RegisterService(&m_service);
    m_completionQueue = builder.AddCompletionQueue();

    m_completionQueueThread = std::thread([this] {
        waitForRPCResponse();
    });
}

void RPCBlockchainServer::waitForRPCResponse() {
    ASSERT(!isSingleThread(), m_environment.logger())

    void* pTag;
    bool ok;
    while (m_completionQueue->Next(&pTag, &ok)) {
        auto* pQuery = static_cast<ServerRPCTag*>(pTag);
        pQuery->process(ok);
        delete pQuery;
    }
}

RPCBlockchainServer::~RPCBlockchainServer() {
    ASSERT(isSingleThread(), m_environment.logger())

    m_blockchainQueries.clear();
    m_blockServerRequest.reset();

    m_completionQueue->Shutdown();

    if (m_completionQueueThread.joinable()) {
        m_completionQueueThread.join();
    }
}

void RPCBlockchainServer::acceptBlockRequest() {
    ASSERT(isSingleThread(), m_environment.logger())

    auto[query, callback] = createAsyncQuery<std::unique_ptr<BlockContext>>([this](auto&& context) {
        ASSERT(context, m_environment.logger())
    }, [] {}, m_environment, true, true);

    m_blockServerRequest = std::move(query);
    auto* tag = new AcceptRequestTag<BlockContext>(std::move(callback));
    m_service.RequestBlock(&tag->m_context->m_context,
                           &tag->m_context->m_request,
                           &tag->m_context->m_responder,
                           m_completionQueue.get(),
                           m_completionQueue.get(),
                           tag);
}

void RPCBlockchainServer::onBlockRequestReceived(std::unique_ptr<BlockContext>&& context) {
    ASSERT(isSingleThread(), m_environment.logger())

    uint64_t queryId = m_blockchainQueriesCounter++;
    auto[query, callback] = createAsyncQuery<blockchain::Block>(
            [this, queryId, context = std::move(context)](auto&& block) mutable {
                ASSERT(block, m_environment.logger());
                onBlockReceived(queryId, std::move(context), *block);
            }, [] {}, m_environment, true, true);
    m_blockchainQueries[queryId] = std::move(query);
    m_blockchain->block(context->m_request.height(), std::move(callback));

    acceptBlockRequest();
}

void
RPCBlockchainServer::onBlockReceived(uint64_t queryId, std::unique_ptr<BlockContext>&& context, const Block& block) {
    ASSERT(isSingleThread(), m_environment.logger())

    m_blockchainQueries.erase(queryId);

    auto* tag = new FinishRequestTag<BlockContext>(std::move(context));
    blockchainServer::BlockResponse response;
    response.set_block_hash(std::string(block.m_blockHash.begin(), block.m_blockHash.end()));
    response.set_block_time(block.m_blockTime);
    tag->m_context->m_responder.Finish(response, grpc::Status::OK, tag);
}

}