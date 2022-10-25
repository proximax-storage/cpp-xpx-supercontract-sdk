/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "AbsolutePathTag.h"

namespace sirius::contract::storage {

AbsolutePathTag::AbsolutePathTag(
        GlobalEnvironment &environment,
        storageServer::AbsolutePathRequest &&request,
        storageServer::StorageServer::Stub &stub,
        grpc::CompletionQueue &completionQueue,
        std::shared_ptr<AsyncQueryCallback<std::string>> &&callback)
        : m_environment(environment), m_request(std::move(request)),
          m_responseReader(stub.PrepareAsyncGetAbsolutePath(&m_context, m_request, &completionQueue)),
          m_callback(std::move(callback)) {}

void AbsolutePathTag::start() {
    m_responseReader->StartCall();
    m_responseReader->Finish(&m_response, &m_status, this);
}

void AbsolutePathTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    ASSERT(ok, m_environment.logger())

    if (!m_status.ok()) {
        m_environment.logger().warn("Failed to obtain the absolute path: {}", m_status.error_message());
        auto error = tl::unexpected<std::error_code>(std::make_error_code(std::errc::connection_aborted));
        m_callback->postReply(error);
    } else if (m_response.absolute_path().empty()) {
        auto error = tl::unexpected<std::error_code>(std::make_error_code(std::errc::no_such_file_or_directory));
        m_callback->postReply(error);
    } else {
        m_callback->postReply(m_response.absolute_path());
    }
}

void AbsolutePathTag::cancel() {
    ASSERT(isSingleThread(), m_environment.logger())

    m_context.TryCancel();
}

}
