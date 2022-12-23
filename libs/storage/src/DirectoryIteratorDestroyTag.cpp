/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "DirectoryIteratorDestroyTag.h"
#include "storage/StorageErrorCode.h"

namespace sirius::contract::storage {

DirectoryIteratorDestroyTag::DirectoryIteratorDestroyTag(
        GlobalEnvironment& environment,
        storageServer::DirectoryIteratorDestroyRequest&& request,
        storageServer::StorageServer::Stub& stub,
        grpc::CompletionQueue& completionQueue,
        std::shared_ptr<AsyncQueryCallback<void>>&& callback)
        : m_environment(environment), m_request(std::move(request)),
          m_responseReader(stub.PrepareAsyncDirectoryIteratorDestroy(&m_context, m_request, &completionQueue)),
          m_callback(std::move(callback)) {}

void DirectoryIteratorDestroyTag::start() {
    m_responseReader->StartCall();
    m_responseReader->Finish(&m_response, &m_status, this);
}

void DirectoryIteratorDestroyTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    ASSERT(ok, m_environment.logger())

    if (!m_status.ok()) {
        m_environment.logger().warn("Failed to destroy directory iterator: {}", m_status.error_message());
        m_callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
    } else if (!m_response.success()) {
        m_callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::destroy_iterator_error)));
    }
    else {
        m_callback->postReply(expected<void>());
    }
}

void DirectoryIteratorDestroyTag::cancel() {
    ASSERT(isSingleThread(), m_environment.logger())

    m_context.TryCancel();
}

}
