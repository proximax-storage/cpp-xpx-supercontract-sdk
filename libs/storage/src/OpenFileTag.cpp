/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "OpenFileTag.h"

namespace sirius::contract::storage {

OpenFileTag::OpenFileTag(
        GlobalEnvironment& environment,
        rpc::OpenFileRequest&& request,
        rpc::StorageContentManagerServer::Stub& stub,
        grpc::CompletionQueue& completionQueue,
        std::shared_ptr<AsyncQueryCallback<uint64_t>>&& callback)
        : m_environment(environment)
        , m_request(std::move(request))
        , m_responseReader(stub.PrepareAsyncOpenFile(&m_context, m_request, &completionQueue))
        , m_callback(std::move(callback)) {}

void OpenFileTag::start() {
    m_responseReader->StartCall();
    m_responseReader->Finish(&m_response, &m_status, this);
}

void OpenFileTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    ASSERT(ok, m_environment.logger())

    if (!m_status.ok()) {
        m_environment.logger().warn("Failed to obtain the absolute path: {}", m_status.error_message());
        m_callback->postReply(
                tl::unexpected<std::error_code>(std::make_error_code(std::errc::no_such_file_or_directory)));
    } else if (!m_response.success()) {
        m_callback->postReply(tl::unexpected<std::error_code>(std::make_error_code(std::errc::io_error)));
    } else {
        m_callback->postReply(m_response.id());
    }
}

}
