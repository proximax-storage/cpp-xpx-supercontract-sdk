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
#include "storage/DirectoryIteratorInfo.h"
#include "storageServer.grpc.pb.h"

namespace sirius::contract::storage {


class DirectoryIteratorNextTag
        : private SingleThread, public RPCTag {

private:

    GlobalEnvironment& m_environment;

    storageServer::DirectoryIteratorNextRequest m_request;
    storageServer::DirectoryIteratorNextResponse m_response;

    grpc::ClientContext m_context;
    std::unique_ptr<
            grpc::ClientAsyncResponseReader<
                    storageServer::DirectoryIteratorNextResponse>> m_responseReader;

    grpc::Status m_status;

    std::shared_ptr<AsyncQueryCallback<DirectoryIteratorInfo>> m_callback;

public:

    DirectoryIteratorNextTag(GlobalEnvironment& environment,
                             storageServer::DirectoryIteratorNextRequest&& request,
                             storageServer::StorageServer::Stub& stub,
                             grpc::CompletionQueue& completionQueue,
                             std::shared_ptr<AsyncQueryCallback<DirectoryIteratorInfo>>&& callback);

    void start();

    void process(bool ok) override;

    void cancel() override;
};

}
