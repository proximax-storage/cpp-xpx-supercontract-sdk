/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ApplySandboxStorageModificationsTag.h"
#include "storage/StorageErrorCode.h"

namespace sirius::contract::storage {

ApplySandboxStorageModificationsTag::ApplySandboxStorageModificationsTag(
        GlobalEnvironment &environment,
        storageServer::ApplySandboxModificationsRequest &&request,
        storageServer::StorageServer::Stub &stub,
        grpc::CompletionQueue &completionQueue,
        std::shared_ptr<AsyncQueryCallback<SandboxModificationDigest>> &&callback)
        : m_environment(environment), m_request(std::move(request)),
          m_responseReader(stub.PrepareAsyncApplySandboxStorageModifications(&m_context, m_request, &completionQueue)),
          m_callback(std::move(callback)) {}

void ApplySandboxStorageModificationsTag::start() {
    m_responseReader->StartCall();
    m_responseReader->Finish(&m_response, &m_status, this);
}

void ApplySandboxStorageModificationsTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    ASSERT(ok, m_environment.logger())

    if (m_status.ok()) {
        SandboxModificationDigest response{m_response.success(), m_response.sandbox_size_delta(),
                                           m_response.state_size_delta()};
        m_callback->postReply(std::move(response));
    } else {
        m_environment.logger().warn("Failed To Apply Sandbox Modifications: {}", m_status.error_message());
        m_callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
    }
}

void ApplySandboxStorageModificationsTag::cancel() {
    ASSERT(isSingleThread(), m_environment.logger())

    m_context.TryCancel();
}

}
