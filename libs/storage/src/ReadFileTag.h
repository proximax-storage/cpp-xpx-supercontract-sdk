///*
//*** Copyright 2021 ProximaX Limited. All rights reserved.
//*** Use of this source code is governed by the Apache 2.0
//*** license that can be found in the LICENSE file.
//*/
//
//#pragma once
//
//#include "RPCTag.h"
//#include "supercontract/SingleThread.h"
//#include "supercontract/GlobalEnvironment.h"
//#include "supercontract/AsyncQuery.h"
//#include "storage/StorageRequests.h"
//#include "storageContentManagerServer.grpc.pb.h"
//
//namespace sirius::contract::storage {
//
//namespace rpc = ::storage;
//
//class ReadFileTag
//        : private SingleThread, public RPCTag {
//
//private:
//
//    GlobalEnvironment& m_environment;
//
//    std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> m_callback;
//
//    rpc::ReadFileRequest   m_request;
//    rpc::ReadFileResponse  m_response;
//
//    grpc::ClientContext m_context;
//    std::unique_ptr<
//            grpc::ClientAsyncResponseReader<
//                    rpc::ReadFileResponse>> m_responseReader;
//
//    grpc::Status m_status;
//
//public:
//
//    ReadFileTag(GlobalEnvironment& environment,
//                                        rpc::ReadFileRequest&& request,
//                                        rpc::StorageContentManagerServer::Stub& stub,
//                                        grpc::CompletionQueue& completionQueue,
//                                        std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>>&& callback);
//
//    void start();
//
//    void process(bool ok) override;
//
//};
//
//}
