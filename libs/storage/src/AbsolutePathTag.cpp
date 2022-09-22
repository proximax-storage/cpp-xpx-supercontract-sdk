/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "AbsolutePathTag.h"

namespace sirius::contract::storage {

AbsolutePathTag::AbsolutePathTag(
        GlobalEnvironment& environment,
        rpc::AbsolutePathRequest&& request,
        rpc::StorageContentManagerServer::Stub& stub,
        grpc::CompletionQueue& completionQueue,
        std::shared_ptr<AsyncQueryCallback<std::string>>&& callback)
        : m_environment(environment)
        , m_request(std::move(request))
        , m_responseReader(stub.PrepareAsyncGetAbsolutePath(&m_context, m_request, &completionQueue))
        , m_callback(std::move(callback)) {}

void AbsolutePathTag::start() {
    m_responseReader->StartCall();
    m_responseReader->Finish(&m_response, &m_status, this);
}

void AbsolutePathTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    ASSERT(ok, m_environment.logger())

    if (m_status.ok()) {
        m_callback->postReply(m_response.absolute_path());
    } else {
        m_environment.logger().warn("Failed to obtain the absolute path: {}", m_status.error_message());
        m_callback->postReply({});
    }
}

}
