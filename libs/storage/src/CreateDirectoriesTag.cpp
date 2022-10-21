/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "CreateDirectoriesTag.h"

namespace sirius::contract::storage {

CreateDirectoriesTag::CreateDirectoriesTag(
        GlobalEnvironment& environment,
        storageServer::CreateDirectoriesRequest&& request,
        storageServer::StorageServer::Stub& stub,
        grpc::CompletionQueue& completionQueue,
        std::shared_ptr<AsyncQueryCallback<void>>&& callback)
        : m_environment(environment), m_request(std::move(request)),
          m_responseReader(stub.PrepareAsyncCreateDirectories(&m_context, m_request, &completionQueue)),
          m_callback(std::move(callback)) {}

void CreateDirectoriesTag::start() {
    m_responseReader->StartCall();
    m_responseReader->Finish(&m_response, &m_status, this);
}

void CreateDirectoriesTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    ASSERT(ok, m_environment.logger())

    if (!m_status.ok()) {
        m_environment.logger().warn("Failed to create directory: {}", m_status.error_message());
        m_callback->postReply(tl::unexpected<std::error_code>(std::make_error_code(std::errc::connection_aborted)));
    } else if (!m_response.success()) {
        // TODO Maybe change error type
        m_callback->postReply(tl::unexpected<std::error_code>(std::make_error_code(std::errc::io_error)));
    } else {
        m_callback->postReply(expected<void>());
    }
}

void CreateDirectoriesTag::cancel() {
    ASSERT(isSingleThread(), m_environment.logger())

    m_context.TryCancel();
}

}
