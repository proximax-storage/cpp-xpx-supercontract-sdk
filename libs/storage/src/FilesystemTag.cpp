/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "FilesystemTag.h"


namespace sirius::contract::storage {

FilesystemTag::FilesystemTag(
        GlobalEnvironment& environment,
        storageServer::FilesystemRequest&& request,
        storageServer::StorageServer::Stub& stub,
        grpc::CompletionQueue& completionQueue,
        std::shared_ptr<AsyncQueryCallback<std::unique_ptr<Folder>>>&& callback)
        : m_environment(environment), m_request(std::move(request)),
          m_responseReader(stub.PrepareAsyncGetFilesystem(&m_context, m_request, &completionQueue)),
          m_callback(std::move(callback)) {}

void FilesystemTag::start() {
    m_responseReader->StartCall();
    m_responseReader->Finish(&m_response, &m_status, this);
}

void FilesystemTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    ASSERT(ok, m_environment.logger())

    if (!m_status.ok()) {
        m_environment.logger().warn("Failed to obtain the filesystem: {}", m_status.error_message());
        auto error = tl::unexpected<std::error_code>(std::make_error_code(std::errc::connection_aborted));
        m_callback->postReply(error);
    } else {
        auto pFolder = buildFolder(m_response.filesystem());
        m_callback->postReply(std::move(pFolder));
    }
}

void FilesystemTag::cancel() {
    ASSERT(isSingleThread(), m_environment.logger())

    m_context.TryCancel();
}

std::unique_ptr<Folder> FilesystemTag::buildFolder(const storageServer::Folder& folder) {

    auto folderEntry = std::make_unique<Folder>(m_environment, folder.name());

    for (const auto& child: folder.children()) {
        std::unique_ptr<FilesystemEntry> entry;
        if (child.has_file()) {
            entry = buildFile(child.file());
        } else {
            entry = buildFolder(child.folder());
        }
        folderEntry->addChild(std::move(entry));
    }

    return std::move(folderEntry);
}

std::unique_ptr<File> FilesystemTag::buildFile(const storageServer::File& file) {
    return std::make_unique<File>(m_environment, file.name());
}

}
