/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ClientBlockTag.h"
#include <blockchain/BlockchainErrorCode.h>

namespace sirius::contract::blockchain {

ClientBlockTag::ClientBlockTag(
        GlobalEnvironment& environment,
        blockchainServer::BlockRequest&& request,
        blockchainServer::BlockchainServer::Stub& stub,
        grpc::CompletionQueue& completionQueue,
        std::shared_ptr<AsyncQueryCallback<Block>>&& callback)
        : m_environment(environment), m_request(std::move(request)), m_responseReader(
        stub.PrepareAsyncBlock(&m_context, m_request, &completionQueue)), m_callback(std::move(callback)) {}

void ClientBlockTag::start() {
    m_responseReader->StartCall();
    m_responseReader->Finish(&m_response, &m_status, this);
}

void ClientBlockTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    ASSERT(ok, m_environment.logger())

    if (m_status.ok()) {
        m_callback->postReply(Block{
                BlockHash(m_response.block_hash()),
                m_response.block_time()
        });
    } else {
        m_environment.logger().warn("Failed to obtain the block {}", m_status.error_message());
        m_callback->postReply(
                tl::unexpected<std::error_code>(make_error_code(BlockchainError::blockchain_unavailable)));
    }
}

void ClientBlockTag::cancel() {
    ASSERT(isSingleThread(), m_environment.logger())

    m_context.TryCancel();
}

}
