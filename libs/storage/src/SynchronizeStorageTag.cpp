/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "SynchronizeStorageTag.h"

namespace sirius::contract::storage {

SynchronizeStorageTag::SynchronizeStorageTag(
        GlobalEnvironment& environment,
        rpc::SynchronizeStorageRequest&& request,
        rpc::StorageServer::Stub& stub,
        grpc::CompletionQueue& completionQueue,
        std::shared_ptr<AsyncQueryCallback<std::optional<bool>>>&& callback)
        : m_environment(environment)
        , m_request(std::move(request))
        , m_responseReader(stub.PrepareAsyncSynchronizeStorage(&m_context, m_request, &completionQueue))
        , m_callback(std::move(callback)) {}

void SynchronizeStorageTag::start() {
    m_responseReader->StartCall();
    m_responseReader->Finish(&m_response, &m_status, this);
}

void SynchronizeStorageTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    ASSERT(ok, m_environment.logger())

    if (m_status.ok()) {
        m_callback->postReply(m_response.status());
    } else {
        m_environment.logger().warn("Failed To Synchronize Storage: {}", m_status.error_message());
        m_callback->postReply({});
    }
}

}
