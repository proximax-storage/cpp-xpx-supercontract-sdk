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
#include <storage/FilesystemEntry.h>
#include <storage/Folder.h>
#include <storage/File.h>

namespace sirius::contract::storage {


class FilesystemTag
        : private SingleThread, public RPCTag {

private:

    GlobalEnvironment& m_environment;

    storageServer::FilesystemRequest m_request;
    storageServer::FilesystemResponse m_response;

    grpc::ClientContext m_context;
    std::unique_ptr<
            grpc::ClientAsyncResponseReader<
                    storageServer::FilesystemResponse>> m_responseReader;

    grpc::Status m_status;

    std::shared_ptr<AsyncQueryCallback<std::unique_ptr<Folder>>> m_callback;

public:

    FilesystemTag(GlobalEnvironment& environment,
                    storageServer::FilesystemRequest&& request,
                    storageServer::StorageServer::Stub& stub,
                    grpc::CompletionQueue& completionQueue,
                    std::shared_ptr<AsyncQueryCallback<std::unique_ptr<Folder>>>&& callback);

    void start();

    void process(bool ok) override;

    void cancel() override;

private:

    std::unique_ptr<Folder> buildFolder(const storageServer::Folder& folder);
    std::unique_ptr<File> buildFile(const storageServer::File& file);
};

}
