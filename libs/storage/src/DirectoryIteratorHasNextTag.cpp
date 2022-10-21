/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "DirectoryIteratorHasNextTag.h"

namespace sirius::contract::storage {

DirectoryIteratorHasNextTag::DirectoryIteratorHasNextTag(
        GlobalEnvironment& environment,
        storageServer::DirectoryIteratorHasNextRequest&& request,
        storageServer::StorageServer::Stub& stub,
        grpc::CompletionQueue& completionQueue,
        std::shared_ptr<AsyncQueryCallback<bool>>&& callback)
        : m_environment(environment), m_request(std::move(request)),
          m_responseReader(stub.PrepareAsyncDirectoryIteratorHasNext(&m_context, m_request, &completionQueue)),
          m_callback(std::move(callback)) {}

void DirectoryIteratorHasNextTag::start() {
    m_responseReader->StartCall();
    m_responseReader->Finish(&m_response, &m_status, this);
}

void DirectoryIteratorHasNextTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    ASSERT(ok, m_environment.logger())

    if (!m_status.ok()) {
        m_environment.logger().warn("Failed to check directory iterator has next: {}", m_status.error_message());
        m_callback->postReply(tl::unexpected<std::error_code>(std::make_error_code(std::errc::connection_aborted)));
    } else {
        m_callback->postReply(m_response.has_next());
    }
}

void DirectoryIteratorHasNextTag::cancel() {
    ASSERT(isSingleThread(), m_environment.logger())

    m_context.TryCancel();
}

}
