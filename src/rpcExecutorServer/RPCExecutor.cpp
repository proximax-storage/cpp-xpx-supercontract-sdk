/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <rpcExecutorServer/RPCExecutor.h>
#include <boost/dll.hpp>
#include <grpcpp/server_builder.h>
#include "StartRPCTag.h"

namespace sirius::contract::rpcExecutorServer {

namespace bp = boost::process;

class RPCReplicatorException : public std::runtime_error {
public :

    explicit RPCReplicatorException(const std::string& what)
            : std::runtime_error(what) {}

};

RPCExecutor::RPCExecutor() {
}

RPCExecutor::~RPCExecutor() {

    m_serviceServer->Shutdown();
    m_completionQueue->Shutdown();

    if (m_completionQueueThread.joinable()) {
        m_completionQueueThread.join();
    }

    if (m_childReplicatorProcess.joinable()) {
        m_childReplicatorProcess.join();
    }
}

void RPCExecutor::waitForRPCResponse() {
    void* pTag;
    bool ok;
    while (m_completionQueue->Next(&pTag, &ok)) {
        auto* pQuery = static_cast<RPCTag*>(pTag);
        pQuery->process(ok);
        delete pQuery;
    }
}

void RPCExecutor::start(const std::string& executorRPCAddress,
                        const) {

    grpc::ServerBuilder builder;
    builder.AddListeningPort(executorRPCAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(&m_service);
    m_completionQueue = builder.AddCompletionQueue();
    m_serviceServer = builder.BuildAndStart();

    m_completionQueueThread = std::thread([this] {
        waitForRPCResponse();
    });

    std::promise<bool> startPromise;
    auto barrier = startPromise.get_future();
    auto* startTag = new StartRPCTag(std::move(startPromise));

    m_service.RequestRunExecutor(&m_serverContext, &m_stream, m_completionQueue.get(), m_completionQueue.get(),
                                 startTag);

    m_childReplicatorProcess = bp::child(
            boost::dll::program_location().parent_path() / "sirius.contract.rpc_executor_client",
            m_address,
            bp::std_out > bp::null,
            bp::std_err > bp::null,
            bp::std_in < bp::null);

    auto status = barrier.wait_for(std::chrono::seconds(5));
    if (status != std::future_status::ready || !barrier.get()) {
        throw RPCReplicatorException("Start RPC Replicator failed");
    }


}

void RPCExecutor::addContract(const ContractKey& key, AddContractRequest&& request) {

}

void RPCExecutor::addManualCall(const ContractKey& key, ManualCallRequest&& request) {

}

void RPCExecutor::setAutomaticExecutionsEnabledSince(const ContractKey& contractKey, uint64_t blockHeight) {

}

void RPCExecutor::addBlockInfo(uint64_t blockHeight, blockchain::Block&& block) {

}

void RPCExecutor::addBlock(const ContractKey& contractKey, uint64_t height) {

}

void RPCExecutor::removeContract(const ContractKey& key, RemoveRequest&& request) {

}

void RPCExecutor::setExecutors(const ContractKey& key, std::map<ExecutorKey, ExecutorInfo>&& executors) {

}

}