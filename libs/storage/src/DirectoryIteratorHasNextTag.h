/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCTag.h"
#include "supercontract/SingleThread.h"
#include "supercontract/GlobalEnvironment.h"
#include "supercontract/AsyncQuery.h"
#include "storage/StorageRequests.h"
#include "storageServer.grpc.pb.h"

namespace sirius::contract::storage {


class DirectoryIteratorHasNextTag
        : private SingleThread, public RPCTag {

private:

    GlobalEnvironment& m_environment;

    std::shared_ptr<AsyncQueryCallback<bool>> m_callback;

    storageServer::DirectoryIteratorHasNextRequest m_request;
    storageServer::DirectoryIteratorHasNextResponse m_response;

    grpc::ClientContext m_context;
    std::unique_ptr<
            grpc::ClientAsyncResponseReader<
                    storageServer::DirectoryIteratorHasNextResponse>> m_responseReader;

    grpc::Status m_status;

public:

    DirectoryIteratorHasNextTag(GlobalEnvironment& environment,
                                storageServer::DirectoryIteratorHasNextRequest&& request,
                                storageServer::StorageServer::Stub& stub,
                                grpc::CompletionQueue& completionQueue,
                                std::shared_ptr<AsyncQueryCallback<bool>>&& callback);

    void start();

    void process(bool ok) override;

    void cancel() override;
};

}
