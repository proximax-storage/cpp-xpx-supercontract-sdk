/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "IsFileTag.h"
#include "storage/StorageErrorCode.h"

namespace sirius::contract::storage {

IsFileTag::IsFileTag(GlobalEnvironment& environment,
                     storageServer::IsFileRequest&& request,
                     storageServer::StorageServer::Stub& stub,
                     grpc::CompletionQueue& completionQueue,
                     std::shared_ptr<AsyncQueryCallback<bool>>&& callback)
    : m_environment(environment), m_request(std::move(request)),
      m_responseReader(stub.PrepareAsyncIsFile(&m_context, m_request, &completionQueue)),
      m_callback(std::move(callback)) {}

void IsFileTag::start() {
    m_responseReader->StartCall();
    m_responseReader->Finish(&m_response, &m_status, this);
}

void IsFileTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    ASSERT(ok, m_environment.logger())

    if (!m_status.ok()) {
        m_environment.logger().warn("Failed to query path: {}", m_status.error_message());
        m_callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
    } else if (!m_response.is_file()) {
        m_callback->postReply(false);
    } else {
        m_callback->postReply(true);
    }
}

void IsFileTag::cancel() {
    ASSERT(isSingleThread(), m_environment.logger())

    m_context.TryCancel();
}

} // namespace sirius::contract::storage
