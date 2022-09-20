/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "EvaluateStorageHashTag.h"

namespace sirius::contract::storage {

EvaluateStorageHashTag::EvaluateStorageHashTag(
        GlobalEnvironment& environment,
        rpc::EvaluateStorageHashRequest&& request,
        rpc::StorageServer::Stub& stub,
        grpc::CompletionQueue& completionQueue,
        std::shared_ptr<AsyncQueryCallback<std::optional<StorageState>>>&& callback)
        : m_environment(environment)
        , m_request(std::move(request))
        , m_responseReader(stub.PrepareAsyncEvaluateStorageHash(&m_context, m_request, &completionQueue))
        , m_callback(std::move(callback)) {}

void EvaluateStorageHashTag::start() {
    m_responseReader->StartCall();
    m_responseReader->Finish(&m_response, &m_status, this);
}

void EvaluateStorageHashTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    ASSERT(ok, m_environment.logger())

    if (m_status.ok()) {
        StorageState response{m_response.storage_hash(), m_response.used_drive_size(), m_response.meta_files_size(),
                              m_response.file_structure_size()};
        m_callback->postReply(std::move(response));
    } else {
        m_environment.logger().warn("Failed To Evaluate Storage Hash: {}", m_status.error_message());
        m_callback->postReply({});
    }
}

}
