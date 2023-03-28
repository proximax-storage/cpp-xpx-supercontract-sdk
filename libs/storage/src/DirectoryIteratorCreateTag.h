/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCTag.h"
#include <common/SingleThread.h>
#include <common/GlobalEnvironment.h>
#include <common/AsyncQuery.h>
#include "storage/StorageRequests.h"
#include "storageServer.grpc.pb.h"

namespace sirius::contract::storage {


class DirectoryIteratorCreateTag
        : private SingleThread, public RPCTag {

private:

    GlobalEnvironment& m_environment;

    storageServer::DirectoryIteratorCreateRequest m_request;
    storageServer::DirectoryIteratorCreateResponse m_response;

    grpc::ClientContext m_context;
    std::unique_ptr<
            grpc::ClientAsyncResponseReader<
                    storageServer::DirectoryIteratorCreateResponse>> m_responseReader;

    grpc::Status m_status;

    std::shared_ptr<AsyncQueryCallback<uint64_t>> m_callback;

public:

    DirectoryIteratorCreateTag(GlobalEnvironment& environment,
                               storageServer::DirectoryIteratorCreateRequest&& request,
                               storageServer::StorageServer::Stub& stub,
                               grpc::CompletionQueue& completionQueue,
                               std::shared_ptr<AsyncQueryCallback<uint64_t>>&& callback);

    void start();

    void process(bool ok) override;

    void cancel() override;
};

}
