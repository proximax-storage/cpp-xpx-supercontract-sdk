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



class EvaluateStorageHashTag
        : private SingleThread, public RPCTag {

private:

    GlobalEnvironment& m_environment;

    storageServer::EvaluateStorageHashRequest m_request;
    storageServer::EvaluateStorageHashResponse m_response;

    grpc::ClientContext m_context;
    std::unique_ptr<
            grpc::ClientAsyncResponseReader<
                    storageServer::EvaluateStorageHashResponse>> m_responseReader;

    grpc::Status m_status;

    std::shared_ptr<AsyncQueryCallback<StorageState>> m_callback;

public:

    EvaluateStorageHashTag(GlobalEnvironment& environment,
                        storageServer::EvaluateStorageHashRequest&& request,
                        storageServer::StorageServer::Stub& stub,
                        grpc::CompletionQueue& completionQueue,
                        std::shared_ptr<AsyncQueryCallback<StorageState>>&& callback);

    void start();

    void process(bool ok) override;

    void cancel() override;

};

}
