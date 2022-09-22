/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <storage/StorageRequests.h>
#include "RPCTag.h"
#include "supercontract/SingleThread.h"
#include "supercontract/GlobalEnvironment.h"
#include "supercontract/AsyncQuery.h"
#include "storageServer.grpc.pb.h"

namespace sirius::contract::storage {

namespace rpc = ::storage;

class ApplyStorageModificationsTag
        : private SingleThread, public RPCTag {

private:

    GlobalEnvironment& m_environment;

    std::shared_ptr<AsyncQueryCallback<bool>> m_callback;

    rpc::ApplyStorageModificationsRequest  m_request;
    rpc::ApplyStorageModificationsResponse m_response;

    grpc::ClientContext m_context;
    std::unique_ptr<
            grpc::ClientAsyncResponseReader<
            rpc::ApplyStorageModificationsResponse>> m_responseReader;

    grpc::Status m_status;

public:

    ApplyStorageModificationsTag(GlobalEnvironment& environment,
                        rpc::ApplyStorageModificationsRequest&& request,
                        rpc::StorageServer::Stub& stub,
                        grpc::CompletionQueue& completionQueue,
                        std::shared_ptr<AsyncQueryCallback<bool>>&& callback);

    void start();

    void process(bool ok) override;

};

}
