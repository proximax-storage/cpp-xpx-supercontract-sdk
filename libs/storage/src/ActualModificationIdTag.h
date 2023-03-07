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


class ActualModificationIdTag
        : private SingleThread, public RPCTag {

private:

    GlobalEnvironment& m_environment;

    storageServer::ActualModificationIdRequest m_request;
    storageServer::ActualModificationIdResponse m_response;

    grpc::ClientContext m_context;
    std::unique_ptr<
            grpc::ClientAsyncResponseReader<
                    storageServer::ActualModificationIdResponse>> m_responseReader;

    grpc::Status m_status;

    std::shared_ptr<AsyncQueryCallback<ModificationId>> m_callback;

public:

    ActualModificationIdTag(GlobalEnvironment& environment,
                    storageServer::ActualModificationIdRequest&& request,
                    storageServer::StorageServer::Stub& stub,
                    grpc::CompletionQueue& completionQueue,
                    std::shared_ptr<AsyncQueryCallback<ModificationId>>&& callback);

    void start();

    void process(bool ok) override;

    void cancel() override;
};

}
