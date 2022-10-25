/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "storage/RPCTag.h"
#include "supercontract/SingleThread.h"
#include "supercontract/GlobalEnvironment.h"
#include "supercontract/AsyncQuery.h"
#include "storage/StorageRequests.h"
#include "storageServer.grpc.pb.h"

namespace sirius::contract::storage {



class OpenFileTag
        : private SingleThread, public RPCTag {
    
private:

    GlobalEnvironment& m_environment;

    std::shared_ptr<AsyncQueryCallback<uint64_t>> m_callback;

    storageServer::OpenFileRequest   m_request;
    storageServer::OpenFileResponse  m_response;

    grpc::ClientContext m_context;
    std::unique_ptr<
            grpc::ClientAsyncResponseReader<
                    storageServer::OpenFileResponse>> m_responseReader;

    grpc::Status m_status;

public:

    OpenFileTag(GlobalEnvironment& environment,
                                        storageServer::OpenFileRequest&& request,
                                        storageServer::StorageServer::Stub& stub,
                                        grpc::CompletionQueue& completionQueue,
                                        std::shared_ptr<AsyncQueryCallback<uint64_t>>&& callback);

    void start();

    void process(bool ok) override;

    void cancel() override;
};

}
