/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCTag.h"
#include "storage/StorageRequests.h"
#include "storageServer.grpc.pb.h"
#include "supercontract/AsyncQuery.h"
#include "supercontract/GlobalEnvironment.h"
#include "supercontract/SingleThread.h"

namespace sirius::contract::storage {

class PathExistTag
    : private SingleThread,
      public RPCTag {

private:
    GlobalEnvironment& m_environment;

    storageServer::PathExistRequest m_request;
    storageServer::PathExistResponse m_response;

    grpc::ClientContext m_context;
    std::unique_ptr<
        grpc::ClientAsyncResponseReader<
            storageServer::PathExistResponse>>
        m_responseReader;

    grpc::Status m_status;

    std::shared_ptr<AsyncQueryCallback<bool>> m_callback;

public:
    PathExistTag(GlobalEnvironment& environment,
                 storageServer::PathExistRequest&& request,
                 storageServer::StorageServer::Stub& stub,
                 grpc::CompletionQueue& completionQueue,
                 std::shared_ptr<AsyncQueryCallback<bool>>&& callback);

    void start();

    void process(bool ok) override;

    void cancel() override;
};

} // namespace sirius::contract::storage
