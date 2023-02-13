/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ActualModificationIdTag.h"
#include "storage/StorageErrorCode.h"

namespace sirius::contract::storage {

ActualModificationIdTag::ActualModificationIdTag(
        GlobalEnvironment &environment,
        storageServer::ActualModificationIdRequest &&request,
        storageServer::StorageServer::Stub &stub,
        grpc::CompletionQueue &completionQueue,
        std::shared_ptr<AsyncQueryCallback<ModificationId>>&& callback)
        : m_environment(environment), m_request(std::move(request)),
          m_responseReader(stub.PrepareAsyncGetActualModificationId(&m_context, m_request, &completionQueue)),
          m_callback(std::move(callback)) {}

void ActualModificationIdTag::start() {
    m_responseReader->StartCall();
    m_responseReader->Finish(&m_response, &m_status, this);
}

void ActualModificationIdTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    ASSERT(ok, m_environment.logger())

    if (!m_status.ok()) {
        m_environment.logger().warn("Failed to obtain the absolute path: {}", m_status.error_message());
        auto error = tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable));
        m_callback->postReply(error);
    }
    else {
        ASSERT(m_response.modification_id().size() == sizeof(ModificationId), m_environment.logger());
        ModificationId modificationId(m_response.modification_id());
        m_callback->postReply(std::move(modificationId));
    }
}

void ActualModificationIdTag::cancel() {
    ASSERT(isSingleThread(), m_environment.logger())

    m_context.TryCancel();
}

}
