/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "InitiateModificationsTag.h"
#include "storage/StorageErrorCode.h"

namespace sirius::contract::storage {

InitiateModificationsTag::InitiateModificationsTag(GlobalEnvironment& environment,
                                                   storageServer::InitModificationsRequest&& request,
                                                   storageServer::StorageServer::Stub& stub,
                                                   grpc::CompletionQueue& completionQueue,
                                                   std::shared_ptr<AsyncQueryCallback<void>>&& callback)
        : m_environment(environment)
        , m_request(std::move(request))
        , m_responseReader(stub.PrepareAsyncInitiateModifications(&m_context, m_request, &completionQueue))
        , m_callback(std::move(callback)) {}

void InitiateModificationsTag::start() {
    m_responseReader->StartCall();
    m_responseReader->Finish(&m_response, &m_status, this);
}

void InitiateModificationsTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    ASSERT(ok, m_environment.logger())

    if (m_status.ok()) {
        m_callback->postReply(expected<void>());
    } else {
        m_environment.logger().warn("Failed To Execute Initiate Storage Modifications: {}", m_status.error_message());
        m_callback->postReply(tl::unexpected<std::error_code>(make_error_code(StorageError::storage_unavailable)));
    }
}

void InitiateModificationsTag::cancel() {
    ASSERT(isSingleThread(), m_environment.logger())

    m_context.TryCancel();
}


}
