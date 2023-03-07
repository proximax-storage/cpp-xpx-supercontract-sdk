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


class MoveFilesystemEntryTag
        : private SingleThread, public RPCTag {

private:

    GlobalEnvironment& m_environment;

    storageServer::MoveFilesystemEntryRequest m_request;
    storageServer::MoveFilesystemEntryResponse m_response;

    grpc::ClientContext m_context;
    std::unique_ptr<
            grpc::ClientAsyncResponseReader<
                    storageServer::MoveFilesystemEntryResponse>> m_responseReader;

    grpc::Status m_status;

    std::shared_ptr<AsyncQueryCallback<void>> m_callback;

public:

    MoveFilesystemEntryTag(GlobalEnvironment& environment,
                               storageServer::MoveFilesystemEntryRequest&& request,
                               storageServer::StorageServer::Stub& stub,
                               grpc::CompletionQueue& completionQueue,
                               std::shared_ptr<AsyncQueryCallback<void>>&& callback);

    void start();

    void process(bool ok) override;

    void cancel() override;
};

}
