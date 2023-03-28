/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCTag.h"
#include "storage/StorageRequests.h"
#include "storageServer.grpc.pb.h"
#include <common/AsyncQuery.h>
#include <common/GlobalEnvironment.h>
#include <common/SingleThread.h>

namespace sirius::contract::storage {

class FileSizeTag
    : private SingleThread,
      public RPCTag {

private:
    GlobalEnvironment& m_environment;

    storageServer::FileSizeRequest m_request;
    storageServer::FileSizeResponse m_response;

    grpc::ClientContext m_context;
    std::unique_ptr<
        grpc::ClientAsyncResponseReader<
            storageServer::FileSizeResponse>>
        m_responseReader;

    grpc::Status m_status;

    std::shared_ptr<AsyncQueryCallback<uint64_t>> m_callback;

public:
    FileSizeTag(GlobalEnvironment& environment,
              storageServer::FileSizeRequest&& request,
              storageServer::StorageServer::Stub& stub,
              grpc::CompletionQueue& completionQueue,
              std::shared_ptr<AsyncQueryCallback<uint64_t>>&& callback);

    void start();

    void process(bool ok) override;

    void cancel() override;
};

} // namespace sirius::contract::storage
